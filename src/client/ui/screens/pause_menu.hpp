#pragma once

#include "client/ui/screen.hpp"

#include <functional>

class PauseMenu : public Screen {
public:
  struct Actions {
    std::function<void()> openToLan;
    std::function<void()> settings;
    std::function<void()> quit;
  };

  struct Info {
    Actions actions;
    bool    canOpenToLan = false;
  };

private:
  Info m_info;

public:
  explicit PauseMenu(Info info);
  ~PauseMenu() override = default;

  void draw(const ui::Context& ctx) override;

  bool isOpaque() const override { return false; }
};
