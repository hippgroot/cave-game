#pragma once

#include "client/ui/rect.hpp"

#include <span>
#include <vector>

namespace ui {

enum class Anchor : u8 {
  TopLeft,
  Top,
  TopRight,
  Left,
  Center,
  Right,
  BottomLeft,
  Bottom,
  BottomRight
};

enum class Align : u8 {
  Start,
  Center,
  End,
  Stretch
};

vec2 anchorPoint(const Rect& rect, Anchor anchor);
Rect anchor(const Rect& parent, Anchor anchor, vec2 extent, vec2 offset = vec2(0.0f));

vec2 vstackExtent(std::span<const vec2> extents, f32 spacing);
vec2 hstackExtent(std::span<const vec2> extents, f32 spacing);
vec2 vstackExtent(vec2 itemExtent, u32 count, f32 spacing);
vec2 hstackExtent(vec2 itemExtent, u32 count, f32 spacing);

std::vector<Rect> vstack(const Rect& area, std::span<const vec2> extents, f32 spacing, Align align = Align::Center);
std::vector<Rect> hstack(const Rect& area, std::span<const vec2> extents, f32 spacing, Align align = Align::Center);
std::vector<Rect> vstack(const Rect& area, u32 count, f32 spacing);
std::vector<Rect> hstack(const Rect& area, u32 count, f32 spacing);

}
