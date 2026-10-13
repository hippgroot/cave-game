#pragma once

#include "client/settings.hpp"
#include "client/ui/screen.hpp"

#include <functional>

class SettingsMenu : public Screen {
public:
  enum class Tab : u8 {
    General,
    Controls,
    Video,
    Audio
  };

  struct Info {
    Settings&                            settings;
    std::function<void(const Settings&)> apply;
    bool                                 vsyncLocked = false;
    Tab                                  tab         = Tab::General;
  };

private:
  Info     m_info;
  Settings m_initial;
  Tab      m_tab;
  f32      m_tabTime = 0.0f;

public:
  explicit SettingsMenu(Info info);
  ~SettingsMenu() override = default;

  void onExit() override;
  void draw(const ui::Context& ctx) override;

private:
  void drawTabs(const ui::Context& ctx, const ui::Rect& area);
  void drawPage(const ui::Context& ctx, const ui::Rect& area);

  void drawGeneral(const ui::Context& ctx, const ui::Rect& area);
  void drawControls(const ui::Context& ctx, const ui::Rect& area);
  void drawVideo(const ui::Context& ctx, const ui::Rect& area);
  void drawAudio(const ui::Context& ctx, const ui::Rect& area);

  void drawEmpty(const ui::Context& ctx, const ui::Rect& area) const;
};
