#include "client/ui/widgets.hpp"

#include "client/ui/anim.hpp"
#include "client/ui/theme.hpp"

#include <grf/imgui.h>

#include <algorithm>
#include <cmath>

namespace ui {

namespace {

ImVec2 toImVec2(vec2 v) {
  return ImVec2(v.x, v.y);
}

ImU32 toColor(vec4 color) {
  return ImGui::GetColorU32(ImVec4(color.r, color.g, color.b, color.a));
}

Rect snapped(const Rect& rect) {
  return Rect{.min = glm::round(rect.min), .max = glm::round(rect.max)};
}

void outlineRect(ImDrawList* drawList, const Rect& rect, f32 width, ImU32 color) {
  const vec2 a = rect.min;
  const vec2 b = rect.max;
  drawList->AddRectFilled(toImVec2(a), ImVec2(b.x, a.y + width), color);
  drawList->AddRectFilled(ImVec2(a.x, b.y - width), toImVec2(b), color);
  drawList->AddRectFilled(ImVec2(a.x, a.y + width), ImVec2(a.x + width, b.y - width), color);
  drawList->AddRectFilled(ImVec2(b.x - width, a.y + width), ImVec2(b.x, b.y - width), color);
}

}

Fade::Fade(f32 alpha) {
  ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * std::clamp(alpha, 0.0f, 1.0f));
}

Fade::~Fade() {
  ImGui::PopStyleVar();
}

vec2 measureText(ImFont* font, f32 fontSize, std::string_view text) {
  if (font == nullptr) font = ImGui::GetFont();
  const ImVec2 extent = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.data(), text.data() + text.size());
  return {extent.x, extent.y};
}

void box(const Context&, const Rect& rect, vec4 fill, vec4 outline, f32 outlineWidth) {
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  const Rect  bounds   = snapped(rect);
  const f32   width    = std::max(1.0f, std::round(outlineWidth));

  drawList->AddRectFilled(toImVec2(bounds.min), toImVec2(bounds.max), toColor(fill));
  outlineRect(drawList, bounds, width, toColor(outline));
}

void panel(const Context& ctx, const Rect& rect) {
  box(ctx, rect, ctx.theme.fill, ctx.theme.outline, ctx.px(ctx.theme.outlineWidth));
}

void label(const Context& ctx, const Rect& rect, std::string_view text,
  ImFont* font, f32 fontSize, Anchor align, bool shadow) {
  if (font == nullptr) font = ImGui::GetFont();

  const vec2  extent   = measureText(font, fontSize, text);
  const Rect  placed   = snapped(anchor(rect, align, extent));
  const char* begin    = text.data();
  const char* end      = text.data() + text.size();
  ImDrawList* drawList = ImGui::GetWindowDrawList();

  if (shadow) {
    const vec2 offset = glm::round(vec2(ctx.px(3.0f)));
    drawList->AddText(font, fontSize, toImVec2(placed.min + offset), toColor(ctx.theme.shadow), begin, end);
  }
  drawList->AddText(font, fontSize, toImVec2(placed.min), toColor(ctx.theme.text), begin, end);
}

void label(const Context& ctx, const Rect& rect, std::string_view text, Anchor align) {
  label(ctx, rect, text, ctx.theme.bodyFont, ctx.px(ctx.theme.bodySize), align);
}

bool button(const Context& ctx, const Rect& rect, std::string_view text) {
  const Theme& theme = ctx.theme;
  const char*  begin = text.data();
  const char*  end   = text.data() + text.size();

  ImGui::PushID(begin, end);
  const ImGuiID hoverID = ImGui::GetID("##hover");
  const ImGuiID pressID = ImGui::GetID("##press");

  bool pressed = false;
  bool hovered = false;
  bool held    = false;
  if (ctx.interactive && rect.width() >= 1.0f && rect.height() >= 1.0f) {
    ImGui::SetCursorScreenPos(toImVec2(rect.min));
    pressed = ImGui::InvisibleButton("##button", toImVec2(rect.size()), ImGuiButtonFlags_EnableNav);
    hovered = ImGui::IsItemHovered() || (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible);
    held    = ImGui::IsItemActive();
  }
  ImGui::PopID();

  const f32 hover = smoothed(hoverID, hovered ? 1.0f : 0.0f, theme.hoverRate, ctx.dt);
  const f32 press = smoothed(pressID, held ? 1.0f : 0.0f, theme.hoverRate * 2.0f, ctx.dt);

  const Rect visual = rect.translated({ctx.px(theme.hoverShift) * hover, 0.0f});
  const vec4 fill   = glm::mix(glm::mix(theme.fill, theme.fillHovered, hover), theme.fillPressed, press);
  const f32  width  = ctx.px(glm::mix(theme.outlineWidth, theme.outlineWidthHovered, hover));
  box(ctx, visual, fill, theme.outline, width);

  const f32 accent = std::round(ctx.px(6.0f) * hover);
  if (accent >= 1.0f) {
    const Rect bounds = snapped(visual);
    ImGui::GetWindowDrawList()->AddRectFilled(
      toImVec2(bounds.min),
      ImVec2(bounds.min.x + width + accent, bounds.max.y),
      toColor(theme.outline));
  }

  label(ctx, visual, text);
  return pressed;
}

}
