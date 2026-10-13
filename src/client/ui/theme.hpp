#pragma once

#include "core/types.hpp"

#include <filesystem>

struct ImFont;

namespace ui {

struct Theme {
  ImFont* bodyFont  = nullptr;
  ImFont* titleFont = nullptr;

  f32 bodySize  = 22.0f;
  f32 titleSize = 76.0f;

  vec4 fill        = {0.0f, 0.0f, 0.0f, 0.8f};
  vec4 fillHovered = {0.08f, 0.08f, 0.08f, 0.9f};
  vec4 fillPressed = {0.22f, 0.22f, 0.22f, 0.9f};
  vec4 outline     = {1.0f, 1.0f, 1.0f, 1.0f};
  vec4 text        = {1.0f, 1.0f, 1.0f, 1.0f};
  vec4 shadow      = {0.0f, 0.0f, 0.0f, 0.6f};

  f32 disabledAlpha = 0.4f;
  f32 mutedAlpha    = 0.6f;

  f32 outlineWidth        = 2.0f;
  f32 outlineWidthHovered = 3.0f;
  f32 hoverShift          = 10.0f;
  f32 hoverRate           = 18.0f;
  f32 referenceHeight     = 720.0f;
};

Theme loadTheme(const std::filesystem::path& fontPath);

}
