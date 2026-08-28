#pragma once

#include <format>
#include <print>
#include <string_view>

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

inline thread_local Thread t_thread = Thread::main;

} // namespace detail

inline void register_thread(Thread thread) {
  detail::t_thread = thread;
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

}