#pragma once

#include "grf/types.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <format>
#include <functional>
#include <mutex>
#include <optional>
#include <print>
#include <string_view>
#include <stop_token>
#include <thread>

using i8    = grf::i8;
using i16   = grf::i16;
using i32   = grf::i32;
using i64   = grf::i64;
using u8    = grf::u8;
using u16   = grf::u16;
using u32   = grf::u32;
using u64   = grf::u64;
using f32   = grf::f32;
using f64   = grf::f64;
using str   = grf::str;
using vec2  = grf::vec2;
using vec3  = grf::vec3;
using vec4  = grf::vec4;
using ivec2 = grf::ivec2;
using ivec3 = grf::ivec3;
using ivec4 = grf::ivec4;
using uvec2 = grf::uvec2;
using uvec3 = grf::uvec3;
using uvec4 = grf::uvec4;
using mat2  = grf::mat2;
using mat3  = grf::mat3;
using mat4  = grf::mat4;

namespace logging {

enum class Thread {
  main,
  render,
  worker
};

constexpr std::string_view to_string(Thread thread) {
  switch (thread) {
  using enum Thread;
  case main:    return "main";
  case render:  return "render";
  case worker:  return "worker";
  }
  return "unknown";
}

namespace detail {
struct Lifetime {
  using clock = std::chrono::steady_clock;
  clock::time_point start = clock::now();
  Lifetime();
  ~Lifetime();
};

inline thread_local Lifetime  t_lifetime{};
inline thread_local Thread    t_thread = Thread::main;

} // namespace detail

inline void register_thread(Thread thread) {
  detail::t_thread = thread;
  detail::t_lifetime.start = detail::Lifetime::clock::now();
}

template <typename... Args>
void general(std::format_string<Args...> fmt, Args&&... args) {
  std::println("[{}] {}", to_string(detail::t_thread), std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void warning(std::format_string<Args...> fmt, Args&&... args) {
  std::println("[{}] WARNING: {}", to_string(detail::t_thread), std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args) {
  std::println("[{}] ERROR: {}", to_string(detail::t_thread), std::format(fmt, std::forward<Args>(args)...));
}

inline detail::Lifetime::Lifetime() { general("Started"); }
inline detail::Lifetime::~Lifetime() {
  auto uptime = clock::now() - start;
  general("Stopped ({:%T})", std::chrono::round<std::chrono::seconds>(uptime));
}

}

template <typename T>
class Channel {
  std::mutex                  m_mutex;
  std::condition_variable_any m_cv;
  std::deque<T>               m_queue;

public:
  Channel() = default;
  ~Channel() = default;

  Channel(const Channel&) = delete;
  Channel(Channel&&) = delete;
  Channel& operator=(const Channel&) = delete;
  Channel& operator=(Channel&&) = delete;

  void send(T data) {
  {
    std::lock_guard lock(m_mutex);
    m_queue.push_back(std::move(data));
  }
    m_cv.notify_one();
  }

  std::optional<T> receive() {
    std::lock_guard lock(m_mutex);
    if (m_queue.empty()) return std::nullopt;

    T data = std::move(m_queue.front());
    m_queue.pop_front();

    return std::move(data);
  }

  std::optional<T> wait_to_receive(std::stop_token st) {
    std::unique_lock lock(m_mutex);

    m_cv.wait(lock, st, [this]{ return !m_queue.empty(); });
    if (m_queue.empty()) return std::nullopt;

    T data = std::move(m_queue.front());
    m_queue.pop_front();

    return std::move(data);
  }
};

class JobPool {
  static const u32 _threadCount = 4u;
  using Job = std::function<void()>;

  std::mutex                  m_mutex;
  std::condition_variable_any m_cv;
  std::deque<Job>             m_jobs;
  std::vector<std::jthread>   m_threads;

public:
  JobPool() {
    u32 threadCount = std::max(1u, std::thread::hardware_concurrency() - 2u);
    logging::general("Worker threads: {}", threadCount);

    m_threads.resize(threadCount);
    for (auto& thread : m_threads)
      thread = std::jthread([this](std::stop_token st){ loop(st); });
  }

  void submit(Job job) {
  {
    std::lock_guard lock(m_mutex);
    m_jobs.push_back(std::move(job));
  }
    m_cv.notify_one();
  }

private:
  void loop(std::stop_token st) {
    register_thread(logging::Thread::worker);
    while (!st.stop_requested()) {
      std::unique_lock lock(m_mutex);

      m_cv.wait(lock, st, [this]{ return !m_jobs.empty(); });
      if (m_jobs.empty() || st.stop_requested()) return;

      Job job = std::move(m_jobs.front());
      m_jobs.pop_front();

      lock.unlock();

      try {
        job();
      }
      catch (std::exception e) {
        logging::error("Job failed: {}", e.what());
      }
    }
  }
};