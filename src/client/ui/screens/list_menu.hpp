#pragma once

#include "client/ui/screen.hpp"

#include <optional>
#include <span>
#include <string_view>
#include <vector>

class ListMenu : public Screen {
public:
  struct Entry {
    str title;
    str detail;
  };

  struct Action {
    std::string_view label;
    bool             needsSelection = false;
  };

private:
  std::string_view    m_title;
  std::string_view    m_emptyText;
  std::vector<Entry>  m_entries;
  std::optional<size> m_selected;
  size                m_scroll = 0;

public:
  ~ListMenu() override = default;

  void draw(const ui::Context& ctx) override;

protected:
  ListMenu(std::string_view title, std::string_view emptyText);

  virtual std::span<const Action> actions() const                                = 0;
  virtual void                    onAction(const ui::Context& ctx, size action)  = 0;
  virtual void                    onActivate(const ui::Context& ctx, size entry) = 0;

  void                setEntries(std::vector<Entry> entries, std::optional<size> select = std::nullopt);
  std::optional<size> selected() const;

private:
  void drawList(const ui::Context& ctx, const ui::Rect& area);
  void drawActions(const ui::Context& ctx, const ui::Rect& area);

  u32 visibleRows(const ui::Context& ctx, const ui::Rect& area) const;
  u32 actionRows() const;
};
