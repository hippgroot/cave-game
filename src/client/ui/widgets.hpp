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

class IDScope {
public:
  explicit IDScope(size id);
  ~IDScope();

  IDScope(const IDScope&)            = delete;
  IDScope(IDScope&&)                 = delete;
  IDScope& operator=(const IDScope&) = delete;
  IDScope& operator=(IDScope&&)      = delete;
};

vec2 measureText(ImFont* font, f32 fontSize, std::string_view text);

void box(const Context& ctx, const Rect& rect, vec4 fill, vec4 outline, f32 outlineWidth);
void panel(const Context& ctx, const Rect& rect);

void label(const Context& ctx, const Rect& rect, std::string_view text,
  ImFont* font, f32 fontSize, Anchor align = Anchor::Center, bool shadow = false);
void label(const Context& ctx, const Rect& rect, std::string_view text, Anchor align = Anchor::Center);

struct ItemEvent {
  bool pressed   = false;
  bool activated = false;
};

void focusNext();
f32  scrollDelta(const Context& ctx, const Rect& rect);

bool      button(const Context& ctx, const Rect& rect, std::string_view text);
bool      button(const Context& ctx, const Rect& rect, std::string_view text, bool enabled);
bool      tab(const Context& ctx, const Rect& rect, std::string_view text, bool selected);
ItemEvent listItem(const Context& ctx, const Rect& rect, std::string_view title, std::string_view detail, bool selected);
bool      textField(const Context& ctx, const Rect& rect, std::string_view id, str& value,
  std::string_view placeholder, size maxLength);
bool      slider(const Context& ctx, const Rect& rect, std::string_view text, f32& value, f32 min, f32 max, f32 step);

}
