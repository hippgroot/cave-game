#pragma once

#include "grf/types.hpp"

#include <chrono>

#include <format>
#include <print>
#include <string_view>

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
using quat  = glm::quat;

namespace logging {

enum class Thread {
  game,
  render,
  worker
};

constexpr std::string_view to_string(Thread thread) {
  switch (thread) {
  using enum Thread;
  case game:    return "game";
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
inline thread_local Thread    t_thread = Thread::game;

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