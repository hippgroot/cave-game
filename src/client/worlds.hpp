#pragma once

#include "core/types.hpp"

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

struct WorldSave {
  static constexpr size maxNameLength = 32;
  static constexpr size maxSeedLength = 32;

  std::filesystem::path directory;
  str                   name;
  u64                   seed       = 0;
  i64                   lastPlayed = 0;

  static std::vector<WorldSave>   list();
  static std::optional<WorldSave> create(std::string_view name, u64 seed);
  static u64                      parseSeed(std::string_view text);

  bool save() const;
};
