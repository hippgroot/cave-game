#pragma once

#include "grf/gui.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <format>
#include <mutex>
#include <optional>
#include <print>
#include <string_view>
#include <stop_token>

namespace logging {

enum class Thread {
  main,
  render,
  chunkgen
};

constexpr std::string_view to_string(Thread thread) {
  switch (thread) {
  using enum Thread;
  case main:      return "main";
  case render:    return "render";
  case chunkgen:  return "chunkgen";
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

struct FrameData {
  grf::GUI::DrawData guiSnapshot;
};

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