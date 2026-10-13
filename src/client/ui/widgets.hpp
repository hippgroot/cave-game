#pragma once

#include "client/ui/context.hpp"
#include "client/ui/layout.hpp"

#include <string_view>

struct ImFont;

namespace ui {

class Fade {
public:
  explicit Fade(f32 alpha);
  ~Fade();

  Fade(const Fade&)            = delete;
  Fade(Fade&&)                 = delete;
  Fade& operator=(const Fade&) = delete;
  Fade& operator=(Fade&&)      = delete;
};

vec2 measureText(ImFont* font, f32 fontSize, std::string_view text);

void box(const Context& ctx, const Rect& rect, vec4 fill, vec4 outline, f32 outlineWidth);
void panel(const Context& ctx, const Rect& rect);

void label(const Context& ctx, const Rect& rect, std::string_view text,
  ImFont* font, f32 fontSize, Anchor align = Anchor::Center, bool shadow = false);
void label(const Context& ctx, const Rect& rect, std::string_view text, Anchor align = Anchor::Center);

bool button(const Context& ctx, const Rect& rect, std::string_view text);
bool tab(const Context& ctx, const Rect& rect, std::string_view text, bool selected);
bool slider(const Context& ctx, const Rect& rect, std::string_view text, f32& value, f32 min, f32 max, f32 step);

}
