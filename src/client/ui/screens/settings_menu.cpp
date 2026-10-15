#include "client/ui/screens/settings_menu.hpp"

#include "client/ui/anim.hpp"
#include "client/ui/layout.hpp"
#include "client/ui/widgets.hpp"

#include <array>
#include <cmath>
#include <format>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view _title = "SETTINGS";

constexpr std::array<std::pair<SettingsMenu::Tab, std::string_view>, 4> _tabs{{
  {SettingsMenu::Tab::General, "General"},
  {SettingsMenu::Tab::Controls, "Controls"},
  {SettingsMenu::Tab::Video, "Video"},
  {SettingsMenu::Tab::Audio, "Audio"},
}};

constexpr f32  _titleSize      = 52.0f;
constexpr f32  _panelWidth     = 760.0f;
constexpr f32  _tabHeight      = 46.0f;
constexpr f32  _tabSpacing     = 8.0f;
constexpr f32  _tabGap         = 10.0f;
constexpr f32  _pageHeight     = 360.0f;
constexpr f32  _pagePadding    = 28.0f;
constexpr f32  _rowHeight      = 52.0f;
constexpr f32  _rowSpacing     = 12.0f;
constexpr f32  _sectionSpacing = 20.0f;
constexpr vec2 _doneExtent     = {340.0f, 52.0f};
constexpr f32  _enterOffset    = 24.0f;
constexpr f32  _pageOffset     = 16.0f;

constexpr ui::Tween _enter{
  .delay    = 0.0f,
  .duration = 0.35f,
  .easing   = ui::ease::outCubic
};

constexpr ui::Tween _pageIn{
  .delay    = 0.0f,
  .duration = 0.25f,
  .easing   = ui::ease::outCubic
};

std::vector<ui::Rect> rows(const ui::Context& ctx, const ui::Rect& area, u32 count) {
  const std::vector<vec2> extents(count, vec2(area.width(), ctx.px(_rowHeight)));
  return ui::vstack(area, extents, ctx.px(_rowSpacing), ui::Align::Stretch);
}

std::string_view onOff(bool value) {
  return value ? "On" : "Off";
}

}

SettingsMenu::SettingsMenu(Info info)
: m_info(std::move(info)),
  m_initial(m_info.settings),
  m_tab(m_info.tab) {}

void SettingsMenu::onExit() {
  if (m_info.settings != m_initial) m_info.settings.save();
}

void SettingsMenu::draw(const ui::Context& ctx) {
  m_tabTime += ctx.dt;
  const Settings before = m_info.settings;

  ui::Fade   fade(_enter.linear(ctx.time));
  const vec2 lift = {0.0f, ctx.px(_enterOffset) * (1.0f - _enter.at(ctx.time))};

  const f32                 titleSize = ctx.px(_titleSize);
  const f32                 gap       = ctx.px(_sectionSpacing);
  const std::array<vec2, 3> extents{
    ui::measureText(ctx.theme.titleFont, titleSize, _title),
    ctx.px(vec2(_panelWidth, _tabHeight + _tabGap + _pageHeight)),
    ctx.px(_doneExtent),
  };

  const ui::Rect column   = ui::anchor(ctx.viewport, ui::Anchor::Center, ui::vstackExtent(extents, gap));
  const auto     sections = ui::vstack(column, extents, gap);

  ui::label(ctx, sections[0], _title, ctx.theme.titleFont, titleSize, ui::Anchor::Center, true);

  const ui::Rect body = sections[1].translated(lift);
  drawTabs(ctx, ui::Rect{
                  .min = body.min,
                  .max = {body.max.x, body.min.y + ctx.px(_tabHeight)}
                });
  drawPage(ctx, ui::Rect{
                  .min = {body.min.x, body.min.y + ctx.px(_tabHeight + _tabGap)},
                  .max = body.max
                });

  if (ui::button(ctx, sections[2].translated(lift), "Done")) ctx.screens.pop();

  if (m_info.settings != before && m_info.apply) m_info.apply(m_info.settings);
}

void SettingsMenu::drawTabs(const ui::Context& ctx, const ui::Rect& area) {
  const auto rects = ui::hstack(area, static_cast<u32>(_tabs.size()), ctx.px(_tabSpacing));

  for (size i = 0; i < _tabs.size(); ++i) {
    const auto& [tab, label] = _tabs[i];
    if (ui::tab(ctx, rects[i], label, m_tab == tab)) {
      m_tab     = tab;
      m_tabTime = 0.0f;
    }
  }
}

void SettingsMenu::drawPage(const ui::Context& ctx, const ui::Rect& area) {
  ui::panel(ctx, area);

  const f32      t       = _pageIn.at(m_tabTime);
  const ui::Rect content = area.inset(ctx.px(_pagePadding)).translated({ctx.px(_pageOffset) * (1.0f - t), 0.0f});
  ui::Fade       fade(_pageIn.linear(m_tabTime));

  switch (m_tab) {
  case Tab::General:  drawGeneral(ctx, content); break;
  case Tab::Controls: drawControls(ctx, content); break;
  case Tab::Video:    drawVideo(ctx, content); break;
  case Tab::Audio:    drawAudio(ctx, content); break;
  }
}

void SettingsMenu::drawGeneral(const ui::Context& ctx, const ui::Rect& area) {
  Settings::General& general = m_info.settings.general;
  const auto         rects   = rows(ctx, area, 2);

  ui::label(ctx, rects[0], "Player Name", ui::Anchor::Left);
  ui::textField(ctx, rects[1], "player_name", general.playerName, "Player", Settings::General::maxPlayerNameLength);
}

void SettingsMenu::drawControls(const ui::Context& ctx, const ui::Rect& area) {
  Settings::Controls& controls = m_info.settings.controls;
  const auto          rects    = rows(ctx, area, 2);

  ui::slider(ctx, rects[0], std::format("Mouse Sensitivity: {:.0f}%###sensitivity", controls.sensitivity * 100.0f),
    controls.sensitivity, Settings::Controls::minSensitivity, Settings::Controls::maxSensitivity, 0.05f);

  if (ui::button(ctx, rects[1], std::format("Invert Mouse Y: {}###invert_y", onOff(controls.invertY))))
    controls.invertY = !controls.invertY;
}

void SettingsMenu::drawVideo(const ui::Context& ctx, const ui::Rect& area) {
  Settings::Video& video      = m_info.settings.video;
  const auto       rects      = rows(ctx, area, 4);
  const bool       borderless = video.windowMode == grf::Window::Mode::Borderless;

  if (ui::button(ctx, rects[0], std::format("Display Mode: {}###window_mode", borderless ? "Borderless" : "Windowed")))
    video.windowMode = borderless ? grf::Window::Mode::Windowed : grf::Window::Mode::Borderless;

  const bool vsync = video.vsync || m_info.vsyncLocked;
  if (ui::button(ctx, rects[1], std::format("VSync: {}###vsync", onOff(vsync)), !m_info.vsyncLocked))
    video.vsync = !video.vsync;

  ui::slider(ctx, rects[2], std::format("Field of View: {:.0f}###fov", video.fov),
    video.fov, Settings::Video::minFOV, Settings::Video::maxFOV, 1.0f);

  f32 viewDistance = static_cast<f32>(video.viewDistance);
  if (ui::slider(ctx, rects[3], std::format("Render Distance: {} chunks###view_distance", video.viewDistance),
        viewDistance, static_cast<f32>(Settings::Video::minViewDistance), static_cast<f32>(Settings::Video::maxViewDistance), 1.0f))
    video.viewDistance = static_cast<i32>(std::lround(viewDistance));
}

void SettingsMenu::drawAudio(const ui::Context& ctx, const ui::Rect& area) {
  drawEmpty(ctx, area);
}

void SettingsMenu::drawEmpty(const ui::Context& ctx, const ui::Rect& area) const {
  ui::Fade fade(0.5f);
  ui::label(ctx, area, "Nothing here yet");
}
