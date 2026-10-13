#include "client/settings.hpp"

#include "core/files.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>

namespace {

template <typename T>
void read(const YAML::Node& section, const char* key, T& value) {
  if (!section.IsMap()) return;

  const YAML::Node node = section[key];
  if (!node) return;

  try {
    value = node.as<T>();
  } catch (const YAML::Exception&) {
    logging::client.warn("Invalid value for setting '{}', using default", key);
  }
}

const char* toString(grf::Window::Mode mode) {
  switch (mode) {
  case grf::Window::Mode::Windowed:   return "windowed";
  case grf::Window::Mode::Borderless: return "borderless";
  }
  return "windowed";
}

void read(const YAML::Node& section, const char* key, grf::Window::Mode& value) {
  str name;
  read(section, key, name);
  if (name == "windowed") value = grf::Window::Mode::Windowed;
  if (name == "borderless") value = grf::Window::Mode::Borderless;
}

}

std::filesystem::path Settings::path() {
  return paths::config() / "settings.yaml";
}

Settings Settings::load() {
  Settings settings;

  YAML::Node root;
  try {
    root = YAML::LoadFile(path().string());
  } catch (const YAML::BadFile&) {
    return settings;
  } catch (const YAML::Exception& e) {
    logging::client.warn("Failed to parse {}: {}", path().string(), e.what());
    return settings;
  }

  const YAML::Node& document = root;
  if (!document.IsMap()) return settings;

  if (const YAML::Node general = document["general"]) {
    read(general, "player_name", settings.general.playerName);
  }

  if (const YAML::Node controls = document["controls"]) {
    read(controls, "sensitivity", settings.controls.sensitivity);
    read(controls, "invert_y", settings.controls.invertY);
  }

  if (const YAML::Node video = document["video"]) {
    read(video, "window_mode", settings.video.windowMode);
    read(video, "vsync", settings.video.vsync);
    read(video, "fov", settings.video.fov);
  }

  if (settings.general.playerName.size() > General::maxPlayerNameLength)
    settings.general.playerName.resize(General::maxPlayerNameLength);

  settings.controls.sensitivity = std::clamp(settings.controls.sensitivity, Controls::minSensitivity, Controls::maxSensitivity);
  settings.video.fov            = std::clamp(settings.video.fov, Video::minFOV, Video::maxFOV);

  return settings;
}

void Settings::save() const {
  YAML::Emitter out;
  out << YAML::BeginMap;

  out << YAML::Key << "general" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "player_name" << YAML::Value << general.playerName;
  out << YAML::EndMap;

  out << YAML::Key << "controls" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "sensitivity" << YAML::Value << controls.sensitivity;
  out << YAML::Key << "invert_y" << YAML::Value << controls.invertY;
  out << YAML::EndMap;

  out << YAML::Key << "video" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "window_mode" << YAML::Value << toString(video.windowMode);
  out << YAML::Key << "vsync" << YAML::Value << video.vsync;
  out << YAML::Key << "fov" << YAML::Value << video.fov;
  out << YAML::EndMap;

  out << YAML::EndMap;

  files::writeAtomic(path(), out.c_str());
}
