#include "client/ui/screens/status_screen.hpp"

#include "client/ui/anim.hpp"
#include "client/ui/layout.hpp"
#include "client/ui/widgets.hpp"

#include <grf/imgui.h>

#include <array>

namespace {

constexpr f32  _titleSize      = 52.0f;
constexpr f32  _panelWidth     = 560.0f;
constexpr f32  _panelHeight    = 96.0f;
constexpr vec2 _buttonExtent   = {340.0f, 52.0f};
constexpr f32  _sectionSpacing = 20.0f;
constexpr f32  _enterOffset    = 24.0f;

constexpr ui::Tween _enter{
  .delay    = 0.0f,
  .duration = 0.3f,
  .easing   = ui::ease::outCubic
};

}

StatusScreen::StatusScreen(Info info)
: m_info(std::move(info)) {}

void StatusScreen::draw(const ui::Context& ctx) {
  ui::Fade   fade(_enter.linear(ctx.time));
  const vec2 lift = {0.0f, ctx.px(_enterOffset) * (1.0f - _enter.at(ctx.time))};

  const f32                 titleSize = ctx.px(_titleSize);
  const f32                 gap       = ctx.px(_sectionSpacing);
  const std::array<vec2, 3> extents{
    ui::measureText(ctx.theme.titleFont, titleSize, m_info.title),
    ctx.px(vec2(_panelWidth, _panelHeight)),
    ctx.px(_buttonExtent),
  };

  const ui::Rect column   = ui::anchor(ctx.viewport, ui::Anchor::Center, ui::vstackExtent(extents, gap));
  const auto     sections = ui::vstack(column, extents, gap);

  ui::label(ctx, sections[0], m_info.title, ctx.theme.titleFont, titleSize, ui::Anchor::Center, true);

  const ui::Rect body = sections[1].translated(lift);
  ui::panel(ctx, body);
  ui::label(ctx, body, m_info.message);

  const bool escape = ctx.interactive && ImGui::IsKeyPressed(ImGuiKey_Escape, false);
  if ((ui::button(ctx, sections[2].translated(lift), m_info.button) || escape) && m_info.action) m_info.action();
}
