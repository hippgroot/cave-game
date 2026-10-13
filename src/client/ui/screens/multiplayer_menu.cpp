#include "client/ui/screens/multiplayer_menu.hpp"

#include "client/ui/screens/form_menu.hpp"

#include <array>

namespace {

enum ActionID : size {
  Join,
  Direct,
  Add,
  Remove,
  Back
};

constexpr std::array<ListMenu::Action, 5> _actions{{
  {.label = "Join Server", .needsSelection = true},
  {.label = "Direct Connect", .needsSelection = false},
  {.label = "Add Server", .needsSelection = false},
  {.label = "Remove", .needsSelection = true},
  {.label = "Back", .needsSelection = false},
}};

FormMenu::Field addressField() {
  return FormMenu::Field{
    .label       = "Server Address",
    .placeholder = "host:port",
    .value       = "",
    .maxLength   = ServerEntry::maxAddressLength,
    .required    = true
  };
}

}

MultiplayerMenu::MultiplayerMenu(Info info)
: ListMenu("MULTIPLAYER", "No servers added"),
  m_info(std::move(info)) {}

void MultiplayerMenu::onEnter() {
  m_servers = ServerList::load();
  refresh();
}

std::span<const ListMenu::Action> MultiplayerMenu::actions() const {
  return _actions;
}

void MultiplayerMenu::onAction(const ui::Context& ctx, size action) {
  const auto index = selected();

  switch (action) {
  case Join:
    if (index) join(m_servers.servers[*index]);
    break;
  case Direct: openDirectConnect(ctx); break;
  case Add:    openAddServer(ctx); break;
  case Remove:
    if (index) remove(*index);
    break;
  case Back: ctx.screens.pop(); break;
  }
}

void MultiplayerMenu::onActivate(const ui::Context&, size entry) {
  join(m_servers.servers[entry]);
}

void MultiplayerMenu::refresh(std::optional<size> select) {
  std::vector<Entry> entries;
  entries.reserve(m_servers.servers.size());
  for (const ServerEntry& server : m_servers.servers)
    entries.push_back(Entry{.title = server.name, .detail = server.address});

  setEntries(std::move(entries), select);
}

void MultiplayerMenu::join(const ServerEntry& server) {
  if (m_info.join) m_info.join(server);
}

void MultiplayerMenu::remove(size index) {
  if (index >= m_servers.servers.size()) return;

  m_servers.servers.erase(m_servers.servers.begin() + static_cast<std::ptrdiff_t>(index));
  m_servers.save();

  const size count = m_servers.servers.size();
  refresh(count == 0 ? std::nullopt : std::optional<size>(std::min(index, count - 1)));
}

void MultiplayerMenu::openAddServer(const ui::Context& ctx) {
  ctx.screens.push<FormMenu>(FormMenu::Info{
    .title  = "ADD SERVER",
    .fields = {
      FormMenu::Field{
        .label       = "Server Name",
        .placeholder = "Cave Server",
        .value       = "",
        .maxLength   = ServerEntry::maxNameLength,
        .required    = false
      },
      addressField(),
    },
    .confirm = "Add",
    .submit  = [this](std::span<const FormMenu::Field> fields) {
      ServerEntry entry{.name = fields[0].value, .address = fields[1].value};
      if (entry.name.empty()) entry.name = "Cave Server";

      m_servers.servers.push_back(std::move(entry));
      m_servers.save();
      refresh(m_servers.servers.size() - 1);
    }
  });
}

void MultiplayerMenu::openDirectConnect(const ui::Context& ctx) {
  ctx.screens.push<FormMenu>(FormMenu::Info{
    .title   = "DIRECT CONNECT",
    .fields  = {addressField()},
    .confirm = "Connect",
    .submit  = [this](std::span<const FormMenu::Field> fields) {
      join(ServerEntry{.name = fields[0].value, .address = fields[0].value});
    }
  });
}
