#pragma once

#include <cstdlib>
#include <filesystem>

namespace paths {

inline std::filesystem::path environment(const char* name) {
  const char* value = std::getenv(name);
  return value && *value ? std::filesystem::path(value) : std::filesystem::path();
}

inline std::filesystem::path config() {
#if defined(_WIN32)
  std::filesystem::path base = environment("APPDATA");
  if (!base.empty()) return base / "CaveGame";
#elif defined(__APPLE__)
  std::filesystem::path home = environment("HOME");
  if (!home.empty()) return home / "Library" / "Application Support" / "CaveGame";
#else
  std::filesystem::path base = environment("XDG_CONFIG_HOME");
  if (!base.empty()) return base / "cave-game";
  std::filesystem::path home = environment("HOME");
  if (!home.empty()) return home / ".config" / "cave-game";
#endif
  return std::filesystem::current_path();
}

inline std::filesystem::path saves() {
  return config() / "saves";
}

}
