#pragma once

#include "client/ui/screen.hpp"

#include <functional>

class StatusScreen : public Screen {
public:
  struct Info {
    str                   title;
    str                   message;
    str                   button;
    std::function<void()> action;
  };

private:
  Info m_info;

public:
  explicit StatusScreen(Info info);
  ~StatusScreen() override = default;

  void draw(const ui::Context& ctx) override;

  bool closesOnEscape() const override { return false; }
};
