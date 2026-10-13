#pragma once

#include "core/types.hpp"

namespace ui {

struct Rect {
  vec2 min = vec2(0.0f);
  vec2 max = vec2(0.0f);

  static Rect fromSize(vec2 position, vec2 size) {
    return Rect{.min = position, .max = position + size};
  }

  vec2 size() const { return max - min; }
  f32  width() const { return max.x - min.x; }
  f32  height() const { return max.y - min.y; }
  vec2 center() const { return (min + max) * 0.5f; }

  Rect translated(vec2 offset) const {
    return Rect{.min = min + offset, .max = max + offset};
  }

  Rect inset(f32 amount) const {
    return Rect{.min = min + amount, .max = max - amount};
  }

  bool contains(vec2 point) const {
    return point.x >= min.x && point.y >= min.y && point.x < max.x && point.y < max.y;
  }
};

}
