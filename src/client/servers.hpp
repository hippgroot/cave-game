#pragma once

#include "core/types.hpp"

#include <filesystem>
#include <vector>

struct ServerEntry {
  static constexpr size maxNameLength    = 32;
  static constexpr size maxAddressLength = 253;

  str name;
  str address;

  bool operator==(const ServerEntry&) const = default;
};

struct ServerList {
  std::vector<ServerEntry> servers;

  static std::filesystem::path path();
  static ServerList            load();
  void                         save() const;
};
