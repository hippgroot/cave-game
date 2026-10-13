#pragma once

#include "client/ui/screen.hpp"

#include <functional>

class MainMenu : public Screen {
public:
  enum class Entrance : u8 {
    Left,
    Bottom
  };

  struct Actions {
    std::function<void()> singleplayer;
    std::function<void()> multiplayer;
    std::function<void()> settings;
    std::function<void()> exit;
  };

  struct Info {
    Actions  actions;
    Entrance entrance = Entrance::Left;
  };

private:
  Info m_info;

public:
  explicit MainMenu(Info info);
  ~MainMenu() override = default;

  void draw(const ui::Context& ctx) override;

  bool closesOnEscape() const override { return false; }
  bool pausesGame() const override { return false; }

private:
  void drawTitle(const ui::Context& ctx) const;
  void drawButtons(const ui::Context& ctx) const;

  vec2 entranceOffset(const ui::Context& ctx, const ui::Rect& rect) const;
};
