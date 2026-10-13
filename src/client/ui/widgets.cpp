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

std::string_view visibleText(std::string_view text) {
  return text.substr(0, text.find("##"));
}

struct Interaction {
  bool pressed = false;
  bool hovered = false;
  bool held    = false;
};

Interaction interact(const Context& ctx, const Rect& rect) {
  Interaction result;
  if (!ctx.interactive || rect.width() < 1.0f || rect.height() < 1.0f) return result;

  ImGui::SetCursorScreenPos(toImVec2(rect.min));
  result.pressed = ImGui::InvisibleButton("##button", toImVec2(rect.size()), ImGuiButtonFlags_EnableNav);
  result.hovered = ImGui::IsItemHovered() || (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible);
  result.held    = ImGui::IsItemActive();
  return result;
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

  ImGui::PushID(text.data(), text.data() + text.size());
  const ImGuiID     hoverID = ImGui::GetID("##hover");
  const ImGuiID     pressID = ImGui::GetID("##press");
  const Interaction input   = interact(ctx, rect);
  ImGui::PopID();

  const f32 hover = smoothed(hoverID, input.hovered ? 1.0f : 0.0f, theme.hoverRate, ctx.dt);
  const f32 press = smoothed(pressID, input.held ? 1.0f : 0.0f, theme.hoverRate * 2.0f, ctx.dt);

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

  label(ctx, visual, visibleText(text));
  return input.pressed;
}

bool tab(const Context& ctx, const Rect& rect, std::string_view text, bool selected) {
  const Theme& theme = ctx.theme;

  ImGui::PushID(text.data(), text.data() + text.size());
  const ImGuiID     hoverID  = ImGui::GetID("##hover");
  const ImGuiID     selectID = ImGui::GetID("##select");
  const Interaction input    = interact(ctx, rect);
  ImGui::PopID();

  const f32 hover  = smoothed(hoverID, input.hovered && !selected ? 1.0f : 0.0f, theme.hoverRate, ctx.dt);
  const f32 active = smoothed(selectID, selected ? 1.0f : 0.0f, theme.hoverRate, ctx.dt);

  const Rect visual = rect.translated({0.0f, -ctx.px(theme.hoverShift) * 0.4f * std::max(hover, active)});
  const vec4 fill   = glm::mix(glm::mix(theme.fill, theme.fillHovered, hover), theme.fillPressed, active);
  const f32  width  = ctx.px(glm::mix(theme.outlineWidth, theme.outlineWidthHovered, std::max(hover, active)));
  box(ctx, visual, fill, theme.outline, width);

  const f32 accent = std::round(ctx.px(5.0f) * active);
  if (accent >= 1.0f) {
    const Rect bounds = snapped(visual);
    const f32  half   = std::round((bounds.width() * 0.5f) * active);
    const f32  center = std::round(bounds.center().x);
    ImGui::GetWindowDrawList()->AddRectFilled(
      ImVec2(center - half, bounds.max.y - width - accent),
      ImVec2(center + half, bounds.max.y),
      toColor(theme.outline));
  }

  label(ctx, visual, visibleText(text));
  return input.pressed && !selected;
}

bool slider(const Context& ctx, const Rect& rect, std::string_view text, f32& value, f32 min, f32 max, f32 step) {
  const Theme& theme = ctx.theme;

  ImGui::PushID(text.data(), text.data() + text.size());
  const ImGuiID     hoverID = ImGui::GetID("##hover");
  const ImGuiID     fillID  = ImGui::GetID("##fill");
  const Interaction input   = interact(ctx, rect);
  const bool        focused = input.hovered && ImGui::IsItemFocused() && ImGui::GetIO().NavVisible;
  ImGui::PopID();

  f32 next = value;
  if (input.held) {
    const f32 mouse = ImGui::GetIO().MousePos.x;
    next            = min + std::clamp((mouse - rect.min.x) / rect.width(), 0.0f, 1.0f) * (max - min);
  }
  if (focused) {
    const f32 nudge = step > 0.0f ? step : (max - min) * 0.05f;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) next -= nudge;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) next += nudge;
  }
  if (step > 0.0f) next = min + std::round((next - min) / step) * step;
  next = std::clamp(next, min, max);

  const bool changed = next != value;
  value              = next;

  const f32 hover  = smoothed(hoverID, input.hovered || input.held ? 1.0f : 0.0f, theme.hoverRate, ctx.dt);
  const f32 target = max > min ? (value - min) / (max - min) : 0.0f;
  const f32 t      = smoothed(fillID, target, theme.hoverRate * 1.5f, ctx.dt);

  const f32 width = ctx.px(glm::mix(theme.outlineWidth, theme.outlineWidthHovered, hover));
  box(ctx, rect, glm::mix(theme.fill, theme.fillHovered, hover), theme.outline, width);

  const Rect  bounds   = snapped(rect);
  const f32   inner    = bounds.width() - width * 2.0f;
  const f32   edge     = std::round(bounds.min.x + width + inner * t);
  const f32   handle   = std::round(ctx.px(4.0f) + ctx.px(2.0f) * hover);
  ImDrawList* drawList = ImGui::GetWindowDrawList();

  drawList->AddRectFilled(
    ImVec2(bounds.min.x + width, bounds.min.y + width),
    ImVec2(edge, bounds.max.y - width),
    toColor(theme.fillPressed));
  drawList->AddRectFilled(
    ImVec2(std::max(bounds.min.x, edge - handle), bounds.min.y),
    ImVec2(std::min(bounds.max.x, edge + handle), bounds.max.y),
    toColor(theme.outline));

  label(ctx, rect, visibleText(text), theme.bodyFont, ctx.px(theme.bodySize), Anchor::Center, true);
  return changed;
}

}
