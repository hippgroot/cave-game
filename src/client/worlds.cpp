#include "client/worlds.hpp"

#include "core/files.hpp"
#include "core/paths.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <bit>
#include <cctype>
#include <charconv>
#include <chrono>
#include <format>
#include <functional>
#include <print>
#include <random>

namespace {

constexpr std::string_view _metadataFile = "world.yaml";
constexpr std::string_view _defaultName  = "New World";

i64 now() {
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::optional<WorldSave> load(const std::filesystem::path& directory) {
  YAML::Node root;
  try {
    root = YAML::LoadFile((directory / _metadataFile).string());
  } catch (const YAML::Exception&) {
    return std::nullopt;
  }
  if (!root.IsMap()) return std::nullopt;

  WorldSave world{.directory = directory, .name = directory.filename().string()};
  try {
    if (root["name"]) world.name = root["name"].as<str>();
    if (root["seed"]) world.seed = root["seed"].as<u64>();
    if (root["last_played"]) world.lastPlayed = root["last_played"].as<i64>();
  } catch (const YAML::Exception&) {
    std::println(stderr, "Invalid world metadata in {}", directory.string());
  }
  return world;
}

str trimmed(std::string_view text) {
  const auto blank = [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; };
  while (!text.empty() && blank(text.front()))
    text.remove_prefix(1);
  while (!text.empty() && blank(text.back()))
    text.remove_suffix(1);
  return str(text);
}

str folderName(std::string_view name) {
  str result;
  for (const char c : name) {
    const bool safe = std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_';
    result += safe ? c : '_';
  }
  result = trimmed(result);
  return result.empty() ? str(_defaultName) : result;
}

}

std::vector<WorldSave> WorldSave::list() {
  std::vector<WorldSave> worlds;

  std::error_code                     error;
  std::filesystem::directory_iterator it(paths::saves(), error);
  for (; !error && it != std::filesystem::directory_iterator(); it.increment(error)) {
    if (!it->is_directory(error)) continue;
    if (auto world = load(it->path())) worlds.push_back(std::move(*world));
  }

  std::ranges::sort(worlds, std::greater{}, &WorldSave::lastPlayed);
  return worlds;
}

std::optional<WorldSave> WorldSave::create(std::string_view name, u64 seed) {
  const str displayName = trimmed(name).empty() ? str(_defaultName) : trimmed(name);
  const str base        = folderName(displayName);

  std::error_code       error;
  std::filesystem::path directory = paths::saves() / base;
  for (u32 i = 2; std::filesystem::exists(directory, error); ++i)
    directory = paths::saves() / std::format("{} ({})", base, i);

  std::filesystem::create_directories(directory, error);
  if (error) {
    std::println(stderr, "Failed to create world directory {}: {}", directory.string(), error.message());
    return std::nullopt;
  }

  WorldSave world{
    .directory  = directory,
    .name       = displayName,
    .seed       = seed,
    .lastPlayed = now()
  };
  if (!world.save()) return std::nullopt;
  return world;
}

u64 WorldSave::parseSeed(std::string_view text) {
  const str seed = trimmed(text);
  if (seed.empty()) {
    std::random_device device;
    return std::uniform_int_distribution<u64>()(device);
  }

  i64 number              = 0;
  const auto [end, error] = std::from_chars(seed.data(), seed.data() + seed.size(), number);
  if (error == std::errc() && end == seed.data() + seed.size()) return std::bit_cast<u64>(number);

  u64 hash = 0xcbf29ce484222325ull;
  for (const char c : seed) {
    hash ^= static_cast<u8>(c);
    hash *= 0x100000001b3ull;
  }
  return hash;
}

bool WorldSave::save() const {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "name" << YAML::Value << name;
  out << YAML::Key << "seed" << YAML::Value << seed;
  out << YAML::Key << "last_played" << YAML::Value << lastPlayed;
  out << YAML::EndMap;

  return files::writeAtomic(directory / _metadataFile, out.c_str());
}
