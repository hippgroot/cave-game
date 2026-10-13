#include "client/ui/screens/singleplayer_menu.hpp"

#include "client/ui/screens/form_menu.hpp"

#include <array>
#include <ctime>
#include <format>

namespace {

enum ActionID : size {
  Play,
  Create,
  Back
};

constexpr std::array<ListMenu::Action, 3> _actions{{
  {.label = "Play World", .needsSelection = true},
  {.label = "Create World", .needsSelection = false},
  {.label = "Back", .needsSelection = false},
}};

str lastPlayed(i64 seconds) {
  if (seconds <= 0) return {};

  const std::time_t time = static_cast<std::time_t>(seconds);
  std::tm           local{};
  if (localtime_r(&time, &local) == nullptr) return {};

  return std::format("{:04}-{:02}-{:02} {:02}:{:02}", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min);
}

}

SingleplayerMenu::SingleplayerMenu(Info info)
: ListMenu("SINGLEPLAYER", "No worlds yet"),
  m_info(std::move(info)) {}

void SingleplayerMenu::onEnter() {
  refresh();
}

std::span<const ListMenu::Action> SingleplayerMenu::actions() const {
  return _actions;
}

void SingleplayerMenu::onAction(const ui::Context& ctx, size action) {
  switch (action) {
  case Play:
    if (const auto index = selected()) play(*index);
    break;
  case Create: openCreateWorld(ctx); break;
  case Back:   ctx.screens.pop(); break;
  }
}

void SingleplayerMenu::onActivate(const ui::Context&, size entry) {
  play(entry);
}

void SingleplayerMenu::refresh(const std::filesystem::path& select) {
  m_worlds = WorldSave::list();

  std::vector<Entry>  entries;
  std::optional<size> selection;
  entries.reserve(m_worlds.size());
  for (const WorldSave& world : m_worlds) {
    if (!select.empty() && world.directory == select) selection = entries.size();
    entries.push_back(Entry{.title = world.name, .detail = lastPlayed(world.lastPlayed)});
  }

  setEntries(std::move(entries), selection);
}

void SingleplayerMenu::play(size index) {
  if (index >= m_worlds.size() || !m_info.play) return;
  m_info.play(m_worlds[index]);
}

void SingleplayerMenu::openCreateWorld(const ui::Context& ctx) {
  ctx.screens.push<FormMenu>(FormMenu::Info{
    .title  = "CREATE WORLD",
    .fields = {
      FormMenu::Field{
        .label       = "World Name",
        .placeholder = "New World",
        .value       = "",
        .maxLength   = WorldSave::maxNameLength,
        .required    = false
      },
      FormMenu::Field{
        .label       = "Seed",
        .placeholder = "Leave blank for random",
        .value       = "",
        .maxLength   = WorldSave::maxSeedLength,
        .required    = false
      },
    },
    .confirm = "Create",
    .submit  = [this](std::span<const FormMenu::Field> fields) {
      if (const auto world = WorldSave::create(fields[0].value, WorldSave::parseSeed(fields[1].value)))
        refresh(world->directory);
    }
  });
}
