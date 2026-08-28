#pragma once

#include "game_objects.hpp"

#include "grf/gui.hpp"

namespace grf {

class Window;

}

class Game {
public:
  struct Info {
    const grf::Window&    window;
    const grf::GUI&       gui;
    const grf::Profiler&  profiler;
  };

  struct Settings {
    f32 fov = glm::radians(70.0f);
    f32 sensitivity = 1.0f;
  };

  struct Data {
    grf::GUI::DrawData  guiSnapshot;
    mat4                cameraProjView;
    Camera::Frustrum    cameraFrustrum;
    vec3                cameraPosition;
  };

private:
  const grf::Window&    m_window;
  const grf::GUI&       m_gui;
  const grf::Profiler&  m_profiler;

  Settings              m_settings;
  Camera                m_camera;

public:
  explicit Game(const Info& info);
  ~Game() = default;

  Game(const Game&) = delete;
  Game(Game&&) = delete;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&) = delete;

  void update(f32 dt);
  void drawGUI();
  Data snapshot() const;
};