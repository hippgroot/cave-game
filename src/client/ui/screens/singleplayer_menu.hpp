#pragma once

#include "client/ui/screens/list_menu.hpp"
#include "client/worlds.hpp"

#include <functional>

class SingleplayerMenu : public ListMenu {
public:
  struct Info {
    std::function<void(const WorldSave&)> play;
  };

private:
  Info                   m_info;
  std::vector<WorldSave> m_worlds;

public:
  explicit SingleplayerMenu(Info info);
  ~SingleplayerMenu() override = default;

  void onEnter() override;

protected:
  std::span<const Action> actions() const override;
  void                    onAction(const ui::Context& ctx, size action) override;
  void                    onActivate(const ui::Context& ctx, size entry) override;

private:
  void refresh(const std::filesystem::path& select = {});
  void play(size index);
  void openCreateWorld(const ui::Context& ctx);
};
