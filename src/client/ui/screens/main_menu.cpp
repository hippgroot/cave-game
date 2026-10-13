#include "client/ui/screens/main_menu.hpp"

#include "client/ui/anim.hpp"
#include "client/ui/layout.hpp"
#include "client/ui/widgets.hpp"

#include <array>
#include <string_view>

namespace {

constexpr std::string_view _title = "CAVE GAME";

constexpr vec2 _buttonExtent  = {340.0f, 52.0f};
constexpr f32  _buttonSpacing = 14.0f;
constexpr f32  _titleMargin   = 96.0f;
constexpr f32  _columnOffset  = 56.0f;
constexpr f32  _offscreenPad  = 32.0f;

constexpr ui::Tween _titleIn{
  .delay    = 0.0f,
  .duration = 0.7f,
  .easing   = ui::ease::outCubic
};

constexpr ui::Tween _buttonIn{
  .delay    = 0.2f,
  .duration = 0.6f,
  .easing   = ui::ease::outBack
};

constexpr f32 _buttonStagger = 0.08f;

}

MainMenu::MainMenu(Info info)
: m_info(std::move(info)) {}

void MainMenu::draw(const ui::Context& ctx) {
  drawTitle(ctx);
  drawButtons(ctx);
}

void MainMenu::drawTitle(const ui::Context& ctx) const {
  const f32  fontSize = ctx.px(ctx.theme.titleSize);
  const vec2 extent   = ui::measureText(ctx.theme.titleFont, fontSize, _title);
  const f32  t        = _titleIn.at(ctx.time);

  const ui::Rect rect = ui::anchor(ctx.viewport, ui::Anchor::Top, extent, {0.0f, ctx.px(_titleMargin)})
                          .translated({0.0f, ctx.px(-40.0f) * (1.0f - t)});

  ui::Fade fade(_titleIn.linear(ctx.time));
  ui::label(ctx, rect, _title, ctx.theme.titleFont, fontSize, ui::Anchor::Center, true);
}

void MainMenu::drawButtons(const ui::Context& ctx) const {
  const std::array<std::pair<std::string_view, const std::function<void()>*>, 4> entries{{
    {"Singleplayer", &m_info.actions.singleplayer},
    {"Multiplayer", &m_info.actions.multiplayer},
    {"Settings", &m_info.actions.settings},
    {"Exit", &m_info.actions.exit},
  }};

  const u32      count  = static_cast<u32>(entries.size());
  const f32      gap    = ctx.px(_buttonSpacing);
  const vec2     extent = ui::vstackExtent(ctx.px(_buttonExtent), count, gap);
  const ui::Rect column = ui::anchor(ctx.viewport, ui::Anchor::Center, extent, {0.0f, ctx.px(_columnOffset)});
  const auto     rows   = ui::vstack(column, count, gap);

  for (u32 i = 0; i < count; ++i) {
    const ui::Tween tween = _buttonIn.staggered(i, _buttonStagger);
    const f32       t     = tween.at(ctx.time);
    const ui::Rect  rect  = rows[i].translated(entranceOffset(ctx, rows[i]) * (1.0f - t));

    ui::Fade fade(tween.linear(ctx.time) * 2.0f);
    const auto& [label, action] = entries[i];
    if (ui::button(ctx, rect, label) && *action) (*action)();
  }
}

vec2 MainMenu::entranceOffset(const ui::Context& ctx, const ui::Rect& rect) const {
  const f32 pad = ctx.px(_offscreenPad);

  switch (m_info.entrance) {
  case Entrance::Left:   return {ctx.viewport.min.x - rect.max.x - pad, 0.0f};
  case Entrance::Bottom: return {0.0f, ctx.viewport.max.y - rect.min.y + pad};
  }
  return vec2(0.0f);
}
