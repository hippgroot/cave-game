#include "client/ui/screens/list_menu.hpp"

#include "client/ui/anim.hpp"
#include "client/ui/layout.hpp"
#include "client/ui/widgets.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

constexpr f32 _titleSize      = 52.0f;
constexpr f32 _panelWidth     = 760.0f;
constexpr f32 _listHeight     = 340.0f;
constexpr f32 _listPadding    = 14.0f;
constexpr f32 _rowHeight      = 56.0f;
constexpr f32 _rowSpacing     = 8.0f;
constexpr f32 _scrollbarWidth = 6.0f;
constexpr f32 _scrollbarGap   = 10.0f;
constexpr f32 _buttonHeight   = 52.0f;
constexpr f32 _buttonSpacing  = 12.0f;
constexpr f32 _sectionSpacing = 20.0f;
constexpr f32 _enterOffset    = 24.0f;
constexpr u32 _actionColumns  = 3;

constexpr ui::Tween _enter{
  .delay    = 0.0f,
  .duration = 0.35f,
  .easing   = ui::ease::outCubic
};

}

ListMenu::ListMenu(std::string_view title, std::string_view emptyText)
: m_title(title),
  m_emptyText(emptyText) {}

void ListMenu::draw(const ui::Context& ctx) {
  ui::Fade   fade(_enter.linear(ctx.time));
  const vec2 lift = {0.0f, ctx.px(_enterOffset) * (1.0f - _enter.at(ctx.time))};

  const u32                 rows      = actionRows();
  const f32                 titleSize = ctx.px(_titleSize);
  const f32                 gap       = ctx.px(_sectionSpacing);
  const std::array<vec2, 3> extents{
    ui::measureText(ctx.theme.titleFont, titleSize, m_title),
    ctx.px(vec2(_panelWidth, _listHeight)),
    ui::vstackExtent(ctx.px(vec2(_panelWidth, _buttonHeight)), rows, ctx.px(_buttonSpacing)),
  };

  const ui::Rect column   = ui::anchor(ctx.viewport, ui::Anchor::Center, ui::vstackExtent(extents, gap));
  const auto     sections = ui::vstack(column, extents, gap);

  ui::label(ctx, sections[0], m_title, ctx.theme.titleFont, titleSize, ui::Anchor::Center, true);
  drawList(ctx, sections[1].translated(lift));
  drawActions(ctx, sections[2].translated(lift));
}

void ListMenu::setEntries(std::vector<Entry> entries, std::optional<size> select) {
  m_entries = std::move(entries);
  if (select && *select >= m_entries.size()) select.reset();
  m_selected = select;
  m_scroll   = select ? *select : std::min(m_scroll, m_entries.size());
}

std::optional<size> ListMenu::selected() const {
  return m_selected;
}

void ListMenu::drawList(const ui::Context& ctx, const ui::Rect& area) {
  ui::panel(ctx, area);

  if (m_entries.empty()) {
    ui::Fade fade(ctx.theme.mutedAlpha);
    ui::label(ctx, area, m_emptyText);
    return;
  }

  const ui::Rect inner     = area.inset(ctx.px(_listPadding));
  const size     visible   = visibleRows(ctx, inner);
  const size     maxScroll = m_entries.size() > visible ? m_entries.size() - visible : 0;
  const bool     overflow  = maxScroll > 0;

  const f32 wheel = ui::scrollDelta(ctx, area);
  if (wheel < 0.0f && m_scroll < maxScroll) ++m_scroll;
  if (wheel > 0.0f && m_scroll > 0) --m_scroll;
  m_scroll = std::min(m_scroll, maxScroll);

  ui::Rect rowsArea = inner;
  if (overflow) rowsArea.max.x -= ctx.px(_scrollbarWidth + _scrollbarGap);

  const size              count = std::min(visible, m_entries.size() - m_scroll);
  const std::vector<vec2> extents(count, vec2(rowsArea.width(), ctx.px(_rowHeight)));
  const auto              rects = ui::vstack(rowsArea, extents, ctx.px(_rowSpacing), ui::Align::Stretch);

  for (size i = 0; i < count; ++i) {
    const size   index = m_scroll + i;
    const Entry& entry = m_entries[index];

    ui::Fade            fade(ui::progress(ctx.time, 0.05f * static_cast<f32>(i), 0.25f));
    ui::IDScope         id(index);
    const ui::ItemEvent event = ui::listItem(ctx, rects[i], entry.title, entry.detail, m_selected == index);

    if (event.pressed) m_selected = index;
    if (event.activated) {
      m_selected = index;
      onActivate(ctx, index);
    }
  }

  if (overflow) {
    const ui::Rect track{
      .min = {inner.max.x - ctx.px(_scrollbarWidth), inner.min.y},
      .max = inner.max
    };
    const f32 total = static_cast<f32>(m_entries.size());
    const f32 top   = track.min.y + track.height() * (static_cast<f32>(m_scroll) / total);
    const f32 thumb = track.height() * (static_cast<f32>(visible) / total);

    ui::box(ctx, track, ctx.theme.fill, ctx.theme.fill, 0.0f);
    ui::box(ctx, ui::Rect{.min = {track.min.x, top}, .max = {track.max.x, top + thumb}}, ctx.theme.outline, ctx.theme.outline, 0.0f);
  }
}

void ListMenu::drawActions(const ui::Context& ctx, const ui::Rect& area) {
  const std::span<const Action> all     = actions();
  const u32                     rows    = actionRows();
  const f32                     spacing = ctx.px(_buttonSpacing);
  const auto                    lines   = ui::vstack(area, rows, spacing);

  for (u32 row = 0; row < rows; ++row) {
    const size first = static_cast<size>(row) * _actionColumns;
    const size count = std::min<size>(_actionColumns, all.size() - first);
    const auto rects = ui::hstack(lines[row], static_cast<u32>(count), spacing);

    for (size i = 0; i < count; ++i) {
      const Action& action  = all[first + i];
      const bool    enabled = !action.needsSelection || m_selected.has_value();
      if (ui::button(ctx, rects[i], action.label, enabled)) onAction(ctx, first + i);
    }
  }
}

u32 ListMenu::visibleRows(const ui::Context& ctx, const ui::Rect& area) const {
  const f32 step = ctx.px(_rowHeight + _rowSpacing);
  return static_cast<u32>(std::max(1.0f, std::floor((area.height() + ctx.px(_rowSpacing)) / step)));
}

u32 ListMenu::actionRows() const {
  const size count = actions().size();
  return static_cast<u32>((count + _actionColumns - 1) / _actionColumns);
}
