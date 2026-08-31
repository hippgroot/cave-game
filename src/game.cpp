#include "game.hpp"

#include "grf/gui.hpp"
#include "grf/window.hpp"

using namespace grf;

Game::Game(const Game::Info& info)
: m_window(info.window),
  m_gui(info.gui),
  m_profiler(info.profiler),
  m_world(info.rand, info.chunkGenerated, info.jobPool)
{
  uvec2 viewport = uvec2(m_window.width(), m_window.height());

  m_camera = Camera(m_settings.fov_deg, viewport, vec3(0.0f, 256.0f, 0.0f), vec2(-45.0f, 0.0f));
  m_world.updateChunks(World::getChunkCoord(m_camera.position()), m_settings.viewDistance);

  m_window.onResize([this](u32 w, u32 h){ m_camera.setViewport(w, h); });
}

void Game::update(f32 dt) {
  processInput(dt);
  drawGUI();
  m_world.updateChunks(World::getChunkCoord(m_camera.position()), m_settings.viewDistance);
}

Renderer::FrameData Game::snapshot() {
  return Renderer::FrameData{
    .guiSnapshot    = m_gui.snapshot(),
    .cameraData     = m_camera.update(),
    .addedMeshes    = m_world.addedChunks()
  };
}

void Game::processInput(f32 dt) {
  static const f32 _speed = 10.0f;
  if (m_gui.ownsInput()) return;

  if (m_window.isPressed(MouseButton::Left)) {
    vec2 delta = m_window.cursorDelta();
    m_camera.rotate(-delta.y * m_settings.sensitivity, -delta.x * m_settings.sensitivity);
  }

  vec3 inputDir = vec3(
    m_window.isPressed(Key::D) - m_window.isPressed(Key::A),
    m_window.isPressed(Key::Space) - m_window.isPressed(Key::LeftShift),
    m_window.isPressed(Key::S) - m_window.isPressed(Key::W)
  );
  if (inputDir != vec3(0.0f))
    m_camera.translate(glm::normalize(inputDir) * _speed * dt);
}

void Game::drawGUI() {
  m_gui.newFrame();
  m_profiler.draw();
}