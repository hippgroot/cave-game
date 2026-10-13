#pragma once

#include "core/types.hpp"

#include <atomic>
#include <chrono>
#include <format>
#include <optional>
#include <string_view>

namespace logging {

enum class Level : u8 {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Off
};

std::string_view     toString(Level level);
std::optional<Level> parseLevel(std::string_view text);

class Category;

void write(const Category& category, Level level, std::string_view message);

class Category {
  std::string_view   m_name;
  std::atomic<Level> m_level;

public:
  constexpr explicit Category(std::string_view name, Level level = Level::Info)
  : m_name(name),
    m_level(level) {}

  Category(const Category&)            = delete;
  Category(Category&&)                 = delete;
  Category& operator=(const Category&) = delete;
  Category& operator=(Category&&)      = delete;

  std::string_view name() const { return m_name; }
  Level            level() const { return m_level.load(std::memory_order_relaxed); }
  void             setLevel(Level level) { m_level.store(level, std::memory_order_relaxed); }
  bool             enabled(Level level) const { return level >= this->level() && level != Level::Off; }

  template <typename... Args>
  void log(Level level, std::format_string<Args...> fmt, Args&&... args) const {
    if (!enabled(level)) return;
    write(*this, level, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void trace(std::format_string<Args...> fmt, Args&&... args) const {
    log(Level::Trace, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void debug(std::format_string<Args...> fmt, Args&&... args) const {
    log(Level::Debug, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void info(std::format_string<Args...> fmt, Args&&... args) const {
    log(Level::Info, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warn(std::format_string<Args...> fmt, Args&&... args) const {
    log(Level::Warn, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void error(std::format_string<Args...> fmt, Args&&... args) const {
    log(Level::Error, fmt, std::forward<Args>(args)...);
  }
};

inline Category core{"core"};
inline Category net{"net"};
inline Category game{"game"};
inline Category client{"client"};
inline Category render{"render"};

void configure(std::string_view spec);
void configureFromEnvironment(const char* variable = "CAVE_LOG");

void             setThreadName(std::string_view name);
std::string_view threadName();

class ThreadScope {
  std::chrono::steady_clock::time_point m_start;

public:
  explicit ThreadScope(std::string_view name);
  ~ThreadScope();

  ThreadScope(const ThreadScope&)            = delete;
  ThreadScope(ThreadScope&&)                 = delete;
  ThreadScope& operator=(const ThreadScope&) = delete;
  ThreadScope& operator=(ThreadScope&&)      = delete;
};

}
