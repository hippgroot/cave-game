#pragma once

#include "core/log.hpp"

#include <filesystem>
#include <fstream>
#include <string_view>

namespace files {

inline bool writeAtomic(const std::filesystem::path& target, std::string_view contents) {
  const std::filesystem::path temp = std::filesystem::path(target).concat(".tmp");

  std::error_code error;
  std::filesystem::create_directories(target.parent_path(), error);
  if (error) {
    logging::core.error("Failed to create {}: {}", target.parent_path().string(), error.message());
    return false;
  }

  {
    std::ofstream file(temp, std::ios::trunc);
    if (!file) {
      logging::core.error("Failed to open {} for writing", temp.string());
      return false;
    }
    file << contents << '\n';
  }

  std::filesystem::rename(temp, target, error);
  if (error) {
    logging::core.error("Failed to save {}: {}", target.string(), error.message());
    return false;
  }
  return true;
}

}
