#pragma once

#include "client/ui/screen.hpp"

#include <functional>
#include <span>
#include <vector>

class FormMenu : public Screen {
public:
  struct Field {
    str  label;
    str  placeholder;
    str  value;
    size maxLength = 32;
    bool required  = false;
  };

  struct Info {
    str                                         title;
    std::vector<Field>                          fields;
    str                                         confirm;
    std::function<void(std::span<const Field>)> submit;
  };

private:
  Info m_info;
  bool m_focusPending = true;

public:
  explicit FormMenu(Info info);
  ~FormMenu() override = default;

  void draw(const ui::Context& ctx) override;

private:
  bool valid() const;
  void submit(const ui::Context& ctx);
};
