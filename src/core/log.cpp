#include "core/log.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <ranges>
#include <string>

#if defined(_WIN32)
#include <io.h>
#define CAVE_ISATTY(fd) _isatty(fd)
#define CAVE_FILENO(file) _fileno(file)
#else
#include <unistd.h>
#define CAVE_ISATTY(fd) isatty(fd)
#define CAVE_FILENO(file) fileno(file)
#endif

namespace logging {

namespace {

using Clock = std::chrono::steady_clock;

const std::array<Category*, 5> _categories{&core, &net, &game, &client, &render};

thread_local std::string t_threadName = "thread";

std::mutex& outputMutex() {
  static std::mutex mutex;
  return mutex;
}

Clock::time_point startTime() {
  static const Clock::time_point start = Clock::now();
  return start;
}

bool colorEnabled() {
  static const bool enabled = CAVE_ISATTY(CAVE_FILENO(stderr)) != 0 && std::getenv("NO_COLOR") == nullptr;
  return enabled;
}

std::string_view color(Level level) {
  switch (level) {
  case Level::Trace: return "\x1b[90m";
  case Level::Debug: return "\x1b[36m";
  case Level::Info:  return "\x1b[32m";
  case Level::Warn:  return "\x1b[33m";
  case Level::Error: return "\x1b[31m";
  case Level::Off:   return "";
  }
  return "";
}

std::string_view trim(std::string_view text) {
  const auto first = text.find_first_not_of(" \t");
  if (first == std::string_view::npos) return {};
  const auto last = text.find_last_not_of(" \t");
  return text.substr(first, last - first + 1);
}

Category* find(std::string_view name) {
  for (Category* category : _categories)
    if (category->name() == name) return category;
  return nullptr;
}

}

std::string_view toString(Level level) {
  switch (level) {
  case Level::Trace: return "TRACE";
  case Level::Debug: return "DEBUG";
  case Level::Info:  return "INFO";
  case Level::Warn:  return "WARN";
  case Level::Error: return "ERROR";
  case Level::Off:   return "OFF";
  }
  return "?";
}

std::optional<Level> parseLevel(std::string_view text) {
  if (text == "trace") return Level::Trace;
  if (text == "debug") return Level::Debug;
  if (text == "info") return Level::Info;
  if (text == "warn") return Level::Warn;
  if (text == "error") return Level::Error;
  if (text == "off") return Level::Off;
  return std::nullopt;
}

void write(const Category& category, Level level, std::string_view message) {
  const f64 elapsed = std::chrono::duration<f64>(Clock::now() - startTime()).count();

  const bool        colored = colorEnabled();
  const std::string line    = std::format("{:9.3f} {:<10} {:<6} {}{:<5}{} {}\n",
    elapsed, t_threadName, category.name(),
    colored ? color(level) : "", toString(level), colored ? "\x1b[0m" : "",
    message);

  std::lock_guard lock(outputMutex());
  std::fwrite(line.data(), 1, line.size(), stderr);
}

void configure(std::string_view spec) {
  for (const auto part : std::views::split(spec, ',')) {
    const std::string_view item = trim(std::string_view(part.begin(), part.end()));
    if (item.empty()) continue;

    const auto separator = item.find('=');
    if (separator == std::string_view::npos) {
      if (const auto level = parseLevel(item)) {
        for (Category* category : _categories)
          category->setLevel(*level);
      } else {
        core.warn("Unknown log level '{}'", item);
      }
      continue;
    }

    const std::string_view name  = trim(item.substr(0, separator));
    const std::string_view value = trim(item.substr(separator + 1));

    Category* category = find(name);
    if (category == nullptr) {
      core.warn("Unknown log category '{}'", name);
      continue;
    }

    if (const auto level = parseLevel(value))
      category->setLevel(*level);
    else
      core.warn("Unknown log level '{}' for category '{}'", value, name);
  }
}

void configureFromEnvironment(const char* variable) {
  startTime();
  if (const char* spec = std::getenv(variable); spec != nullptr && *spec != '\0') configure(spec);
}

void setThreadName(std::string_view name) {
  t_threadName = name;
}

std::string_view threadName() {
  return t_threadName;
}

ThreadScope::ThreadScope(std::string_view name)
: m_start(Clock::now()) {
  setThreadName(name);
  core.info("Thread started");
}

ThreadScope::~ThreadScope() {
  const f64 uptime = std::chrono::duration<f64>(Clock::now() - m_start).count();
  core.info("Thread stopped after {:.2f}s", uptime);
}

}
