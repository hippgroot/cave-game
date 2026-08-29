#pragma once

#include "game_objects.hpp"
#include "world/world.hpp"

#include "grf/gui.hpp"

namespace grf {

class Window;

}

class Game {
public:
  struct Info {
    const grf::Window&      window;
    const grf::GUI&         gui;
    const grf::Profiler&    profiler;
    Random&                 rand;
    Channel<World::GenRes>& chunkGenerated;
    JobPool&                jobPool;
  };

  struct Settings {
    f32 fov = glm::radians(70.0f);
    f32 sensitivity = 1.0f;
    i32 viewDistance = 12;
  };

  struct Data {
    grf::GUI::DrawData  guiSnapshot;
    mat4                cameraProjView;
    Camera::Frustrum    cameraFrustrum;
    vec3                cameraPosition;
    World::AddedChunks  addedChunks;
    std::vector<ivec2>  removedChunks;
  };

private:
  const grf::Window&    m_window;
  const grf::GUI&       m_gui;
  const grf::Profiler&  m_profiler;

  Settings              m_settings;
  World                 m_world;
  Camera                m_camera;

public:
  explicit Game(const Info& info);
  ~Game() = default;

  Game(const Game&) = delete;
  Game(Game&&) = delete;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&) = delete;

  void update(f32 dt);
  Data snapshot();

private:
  void processInput(f32 dt);
  void drawGUI();

};