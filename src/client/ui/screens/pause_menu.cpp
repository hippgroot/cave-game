#include "client/ui/screens/pause_menu.hpp"

#include "client/ui/anim.hpp"
#include "client/ui/layout.hpp"
#include "client/ui/widgets.hpp"

#include <array>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view _title = "GAME PAUSED";

constexpr f32  _titleSize     = 52.0f;
constexpr f32  _titleSpacing  = 36.0f;
constexpr vec2 _buttonExtent  = {340.0f, 52.0f};
constexpr f32  _buttonSpacing = 14.0f;
constexpr f32  _enterOffset   = 24.0f;
constexpr vec4 _dim           = {0.0f, 0.0f, 0.0f, 0.55f};

constexpr ui::Tween _enter{
  .delay    = 0.0f,
  .duration = 0.25f,
  .easing   = ui::ease::outCubic
};

constexpr ui::Tween _buttonIn{
  .delay    = 0.05f,
  .duration = 0.3f,
  .easing   = ui::ease::outCubic
};

constexpr f32 _buttonStagger = 0.04f;

struct Entry {
  std::string_view             label;
  const std::function<void()>* action;
};

}

PauseMenu::PauseMenu(Info info)
: m_info(std::move(info)) {}

void PauseMenu::draw(const ui::Context& ctx) {
  const f32 fade = _enter.linear(ctx.time);
  ui::box(ctx, ctx.viewport, vec4(vec3(_dim), _dim.a * fade), vec4(0.0f), 0.0f);

  const std::function<void()> resume = [&ctx] { ctx.screens.pop(); };

  std::vector<Entry> entries{{"Back to Game", &resume}};
  if (m_info.canOpenToLan) entries.push_back({"Open to LAN", &m_info.actions.openToLan});
  entries.push_back({"Settings", &m_info.actions.settings});
  entries.push_back({"Quit to Title", &m_info.actions.quit});

  const u32  count       = static_cast<u32>(entries.size());
  const f32  titleSize   = ctx.px(_titleSize);
  const vec2 titleExtent = ui::measureText(ctx.theme.titleFont, titleSize, _title);
  const f32  gap         = ctx.px(_buttonSpacing);
  const vec2 stackExtent = ui::vstackExtent(ctx.px(_buttonExtent), count, gap);

  const std::array<vec2, 2> sections{titleExtent, stackExtent};
  const ui::Rect            column = ui::anchor(ctx.viewport, ui::Anchor::Center, ui::vstackExtent(sections, ctx.px(_titleSpacing)));
  const auto                areas  = ui::vstack(column, sections, ctx.px(_titleSpacing));

  {
    ui::Fade       titleFade(fade);
    const ui::Rect title = areas[0].translated({0.0f, ctx.px(-_enterOffset) * (1.0f - _enter.at(ctx.time))});
    ui::label(ctx, title, _title, ctx.theme.titleFont, titleSize, ui::Anchor::Center, true);
  }

  const auto rows = ui::vstack(areas[1], count, gap);
  for (u32 i = 0; i < count; ++i) {
    const ui::Tween tween = _buttonIn.staggered(i, _buttonStagger);
    const ui::Rect  rect  = rows[i].translated({0.0f, ctx.px(_enterOffset) * (1.0f - tween.at(ctx.time))});

    ui::Fade buttonFade(tween.linear(ctx.time));
    const auto& [label, action] = entries[i];
    if (ui::button(ctx, rect, label) && *action) (*action)();
  }
}
