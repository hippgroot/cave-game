#pragma once

#include "core/types.hpp"

#include <grf/window.hpp>

#include <filesystem>

struct Settings {
  struct General {
    static constexpr size maxPlayerNameLength = 16;

    str playerName = "Player";

    bool operator==(const General&) const = default;
  };

  struct Controls {
    static constexpr f32 minSensitivity = 0.1f;
    static constexpr f32 maxSensitivity = 3.0f;

    f32  sensitivity = 1.0f;
    bool invertY     = false;

    bool operator==(const Controls&) const = default;
  };

  struct Video {
    static constexpr f32 minFOV          = 30.0f;
    static constexpr f32 maxFOV          = 110.0f;
    static constexpr i32 minViewDistance = 2;
    static constexpr i32 maxViewDistance = 32;

    grf::Window::Mode windowMode   = grf::Window::Mode::Windowed;
    bool              vsync        = true;
    f32               fov          = 75.0f;
    i32               viewDistance = 8;

    bool operator==(const Video&) const = default;
  };

  struct Audio {
    bool operator==(const Audio&) const = default;
  };

  General  general;
  Controls controls;
  Video    video;
  Audio    audio;

  bool operator==(const Settings&) const = default;

  static std::filesystem::path path();
  static Settings              load();
  void                         save() const;
};
