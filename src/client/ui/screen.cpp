#include "client/ui/screen.hpp"

#include <grf/imgui.h>

#include <algorithm>

namespace {

constexpr ImGuiWindowFlags _screenWindowFlags =
  ImGuiWindowFlags_NoDecoration |
  ImGuiWindowFlags_NoBackground |
  ImGuiWindowFlags_NoMove |
  ImGuiWindowFlags_NoSavedSettings |
  ImGuiWindowFlags_NoBringToFrontOnFocus |
  ImGuiWindowFlags_NoScrollWithMouse;

}

ScreenStack::ScreenStack(ui::Theme theme)
: m_theme(theme) {}

ScreenStack::~ScreenStack() {
  exitAll();
}

void ScreenStack::push(std::unique_ptr<Screen> screen) {
  m_pending.push_back(Op{.type = OpType::Push, .screen = std::move(screen)});
}

void ScreenStack::pop() {
  m_pending.push_back(Op{.type = OpType::Pop, .screen = nullptr});
}

void ScreenStack::replace(std::unique_ptr<Screen> screen) {
  m_pending.push_back(Op{.type = OpType::Replace, .screen = std::move(screen)});
}

void ScreenStack::clear() {
  m_pending.push_back(Op{.type = OpType::Clear, .screen = nullptr});
}

void ScreenStack::draw(f32 dt) {
  flush();

  if (!m_screens.empty() && m_screens.back()->closesOnEscape() &&
      ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !ImGui::GetIO().WantTextInput) {
    pop();
    flush();
  }

  if (m_screens.empty()) return;

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ui::Rect       bounds   = ui::Rect::fromSize(
    {viewport->WorkPos.x, viewport->WorkPos.y},
    {viewport->WorkSize.x, viewport->WorkSize.y});
  const f32 scale = std::max(0.5f, bounds.height() / m_theme.referenceHeight);

  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("##screens", nullptr, _screenWindowFlags);
  ImGui::PopStyleVar(2);

  auto       firstVisible = std::find_if(m_screens.rbegin(), m_screens.rend(), [](const auto& screen) {
    return screen->isOpaque();
  });
  const size first        = firstVisible == m_screens.rend()
                              ? 0
                              : static_cast<size>(std::distance(firstVisible, m_screens.rend()) - 1);

  for (size i = first; i < m_screens.size(); ++i) {
    Screen& screen = *m_screens[i];
    screen.m_time += dt;

    const ui::Context ctx{
      .screens     = *this,
      .theme       = m_theme,
      .viewport    = bounds,
      .dt          = dt,
      .time        = screen.m_time,
      .scale       = scale,
      .interactive = i + 1 == m_screens.size()
    };

    ImGui::PushID(&screen);
    screen.draw(ctx);
    ImGui::PopID();
  }

  ImGui::End();

  flush();
}

bool ScreenStack::empty() const {
  return m_screens.empty();
}

Screen* ScreenStack::top() const {
  return m_screens.empty() ? nullptr : m_screens.back().get();
}

bool ScreenStack::pausesGame() const {
  return std::ranges::any_of(m_screens, [](const auto& screen) {
    return screen->pausesGame();
  });
}

const ui::Theme& ScreenStack::theme() const {
  return m_theme;
}

void ScreenStack::flush() {
  std::vector<Op> pending = std::move(m_pending);
  m_pending.clear();

  for (Op& op : pending) {
    switch (op.type) {
    case OpType::Push:
      enter(std::move(op.screen));
      break;
    case OpType::Pop:
      exitTop();
      break;
    case OpType::Replace:
      if (!m_screens.empty()) {
        m_screens.back()->onExit();
        m_screens.pop_back();
      }
      install(std::move(op.screen));
      break;
    case OpType::Clear:
      exitAll();
      break;
    }
  }

  if (!m_pending.empty()) flush();
}

void ScreenStack::enter(std::unique_ptr<Screen> screen) {
  if (!screen) return;

  if (!m_screens.empty()) m_screens.back()->onCovered();
  install(std::move(screen));
}

void ScreenStack::install(std::unique_ptr<Screen> screen) {
  if (!screen) return;

  screen->m_time = 0.0f;
  m_screens.push_back(std::move(screen));
  m_screens.back()->onEnter();
}

void ScreenStack::exitTop() {
  if (m_screens.empty()) return;

  m_screens.back()->onExit();
  m_screens.pop_back();

  if (!m_screens.empty()) m_screens.back()->onUncovered();
}

void ScreenStack::exitAll() {
  while (!m_screens.empty()) {
    m_screens.back()->onExit();
    m_screens.pop_back();
  }
}
