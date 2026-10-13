#include "client/servers.hpp"

#include "core/files.hpp"
#include "core/paths.hpp"

#include <yaml-cpp/yaml.h>

#include <print>

std::filesystem::path ServerList::path() {
  return paths::config() / "servers.yaml";
}

ServerList ServerList::load() {
  ServerList list;

  YAML::Node root;
  try {
    root = YAML::LoadFile(path().string());
  } catch (const YAML::BadFile&) {
    return list;
  } catch (const YAML::Exception& e) {
    std::println(stderr, "Failed to parse {}: {}", path().string(), e.what());
    return list;
  }

  const YAML::Node servers = root.IsMap() ? root["servers"] : YAML::Node();
  if (!servers.IsSequence()) return list;

  for (const YAML::Node& node : servers) {
    if (!node.IsMap() || !node["address"]) continue;

    try {
      ServerEntry entry{.name = "", .address = node["address"].as<str>()};
      entry.name = node["name"] ? node["name"].as<str>() : entry.address;
      list.servers.push_back(std::move(entry));
    } catch (const YAML::Exception&) {
      std::println(stderr, "Skipping invalid server entry in {}", path().string());
    }
  }

  return list;
}

void ServerList::save() const {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "servers" << YAML::Value << YAML::BeginSeq;
  for (const ServerEntry& entry : servers) {
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << entry.name;
    out << YAML::Key << "address" << YAML::Value << entry.address;
    out << YAML::EndMap;
  }
  out << YAML::EndSeq;
  out << YAML::EndMap;

  files::writeAtomic(path(), out.c_str());
}
