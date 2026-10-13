#include "client/ui/screens/form_menu.hpp"

#include "client/ui/anim.hpp"
#include "client/ui/layout.hpp"
#include "client/ui/widgets.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace {

constexpr f32 _titleSize      = 52.0f;
constexpr f32 _panelWidth     = 560.0f;
constexpr f32 _panelPadding   = 28.0f;
constexpr f32 _labelHeight    = 28.0f;
constexpr f32 _labelGap       = 6.0f;
constexpr f32 _inputHeight    = 52.0f;
constexpr f32 _fieldSpacing   = 18.0f;
constexpr f32 _buttonHeight   = 52.0f;
constexpr f32 _buttonSpacing  = 12.0f;
constexpr f32 _sectionSpacing = 20.0f;
constexpr f32 _enterOffset    = 24.0f;

constexpr ui::Tween _enter{
  .delay    = 0.0f,
  .duration = 0.3f,
  .easing   = ui::ease::outCubic
};

bool blank(const str& value) {
  return std::ranges::all_of(value, [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; });
}

}

FormMenu::FormMenu(Info info)
: m_info(std::move(info)) {}

void FormMenu::draw(const ui::Context& ctx) {
  ui::Fade   fade(_enter.linear(ctx.time));
  const vec2 lift = {0.0f, ctx.px(_enterOffset) * (1.0f - _enter.at(ctx.time))};

  const u32 count       = static_cast<u32>(m_info.fields.size());
  const f32 fieldHeight = _labelHeight + _labelGap + _inputHeight;
  const f32 bodyHeight  = _panelPadding * 2.0f + ui::vstackExtent(vec2(0.0f, fieldHeight), count, _fieldSpacing).y;

  const f32                 titleSize = ctx.px(_titleSize);
  const f32                 gap       = ctx.px(_sectionSpacing);
  const std::array<vec2, 3> extents{
    ui::measureText(ctx.theme.titleFont, titleSize, m_info.title),
    ctx.px(vec2(_panelWidth, bodyHeight)),
    ctx.px(vec2(_panelWidth, _buttonHeight)),
  };

  const ui::Rect column   = ui::anchor(ctx.viewport, ui::Anchor::Center, ui::vstackExtent(extents, gap));
  const auto     sections = ui::vstack(column, extents, gap);

  ui::label(ctx, sections[0], m_info.title, ctx.theme.titleFont, titleSize, ui::Anchor::Center, true);

  const ui::Rect body = sections[1].translated(lift);
  ui::panel(ctx, body);

  const std::vector<vec2> fieldExtents(count, vec2(0.0f, ctx.px(fieldHeight)));
  const auto              fields = ui::vstack(body.inset(ctx.px(_panelPadding)), fieldExtents, ctx.px(_fieldSpacing), ui::Align::Stretch);

  bool entered = false;
  for (u32 i = 0; i < count; ++i) {
    Field&         field = m_info.fields[i];
    const ui::Rect area  = fields[i];

    ui::label(ctx, ui::Rect{.min = area.min, .max = {area.max.x, area.min.y + ctx.px(_labelHeight)}}, field.label, ui::Anchor::Left);

    if (i == 0 && m_focusPending && ctx.interactive) {
      ui::focusNext();
      m_focusPending = false;
    }

    const ui::Rect input{.min = {area.min.x, area.max.y - ctx.px(_inputHeight)}, .max = area.max};
    ui::IDScope    id(i);
    if (ui::textField(ctx, input, field.label, field.value, field.placeholder, field.maxLength)) entered = true;
  }

  const auto buttons = ui::hstack(sections[2].translated(lift), 2, ctx.px(_buttonSpacing));
  if (ui::button(ctx, buttons[0], "Cancel")) ctx.screens.pop();
  if (ui::button(ctx, buttons[1], m_info.confirm, valid()) || (entered && valid())) submit(ctx);
}

bool FormMenu::valid() const {
  return std::ranges::none_of(m_info.fields, [](const Field& field) {
    return field.required && blank(field.value);
  });
}

void FormMenu::submit(const ui::Context& ctx) {
  ctx.screens.pop();
  if (m_info.submit) m_info.submit(m_info.fields);
}
