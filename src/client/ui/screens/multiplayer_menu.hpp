#pragma once

#include "client/servers.hpp"
#include "client/ui/screens/list_menu.hpp"

#include <functional>

class MultiplayerMenu : public ListMenu {
public:
  struct Info {
    std::function<void(const ServerEntry&)> join;
  };

private:
  Info       m_info;
  ServerList m_servers;

public:
  explicit MultiplayerMenu(Info info);
  ~MultiplayerMenu() override = default;

  void onEnter() override;

protected:
  std::span<const Action> actions() const override;
  void                    onAction(const ui::Context& ctx, size action) override;
  void                    onActivate(const ui::Context& ctx, size entry) override;

private:
  void refresh(std::optional<size> select = std::nullopt);
  void join(const ServerEntry& server);
  void remove(size index);
  void openAddServer(const ui::Context& ctx);
  void openDirectConnect(const ui::Context& ctx);
};
