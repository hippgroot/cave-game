#pragma once

#include "client/ui/rect.hpp"

class ScreenStack;

namespace ui {

struct Theme;

struct Context {
  ScreenStack& screens;
  const Theme& theme;
  Rect         viewport;
  f32          dt;
  f32          time;
  f32          scale;
  bool         interactive;

  f32  px(f32 value) const { return value * scale; }
  vec2 px(vec2 value) const { return value * scale; }
};

}
