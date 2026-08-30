#pragma once

#include "camera/camera.hpp"
#include "world/world.hpp"

#include "grf/gui.hpp"

namespace grf {

class Window;

}

class Game {
public:
  struct Info {
    grf::Window&            window;
    const grf::GUI&         gui;
    const grf::Profiler&    profiler;
    Random&                 rand;
    Channel<World::GenRes>& chunkGenerated;
    JobPool&                jobPool;
  };

  struct Settings {
    f32 fov_deg = 70.0f;
    f32 sensitivity = 180.0f;
    i32 viewDistance = 12;
  };

  struct Data {
    grf::GUI::DrawData  guiSnapshot;
    std::optional<Camera::UpdateData> cameraData;
    World::AddedChunks  addedChunks;
    std::vector<ivec2>  removedChunks;
  };

private:
  grf::Window&          m_window;
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