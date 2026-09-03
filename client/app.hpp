#pragma once

#include "engine/core.hpp"
#include "engine/render.hpp"

#include "game/game.hpp"

enum class UIState {
  Closed,
  MainMenu,
  GenerateMenu,
  SettingsMenu
};

class App {
  struct Settings {
    f32 sensitivity = 0.001f;
    f32 fov_deg = 70.0f;
    i32 viewDistance = 12;
  };

  Random&                 m_rng;
  const grf::Window&      m_window;
  const grf::GUI&         m_gui;
  const grf::Profiler&    m_profiler;

  Settings                m_settings{};
  UIState                 m_state = UIState::MainMenu;
  bool                    m_debugEnabled = false;
  bool                    m_exitRequested = false;

  std::unique_ptr<Game>   m_game;
  std::unique_ptr<Camera> m_camera;

public:
  explicit App(Random& rng, const grf::Window& window, const grf::GUI& gui, const grf::Profiler& profiler);
  ~App() = default;

  App(const App&) = delete;
  App(App&&) = delete;
  App& operator=(const App&) = delete;
  App& operator=(App&&) = delete;

  void update(f32 dt);
  bool exitRequested() const;
  std::optional<Camera::UpdateData> cameraData() const;

private:
  void setupGUIStyle() const;

  Game::Input processInput(f32 dt);
  void drawGUI();

  void gui_mainMenu();
  void gui_generateMenu();
  void gui_settingsMenu();
  void gui_debugOverlay();
};