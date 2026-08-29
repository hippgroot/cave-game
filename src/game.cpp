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
  vec2 planePos(m_camera.position().x, m_camera.position().z);
  ivec2 currChunk(glm::floor(planePos / static_cast<f32>(Chunk::size())));
  m_world.updateChunks(currChunk, m_settings.viewDistance);
}

void Game::update(f32 dt) {
  processInput(dt);
  drawGUI();

  vec2 planePos(m_camera.position().x, m_camera.position().z);
  ivec2 currChunk(glm::floor(planePos / static_cast<f32>(Chunk::size())));
  m_world.updateChunks(currChunk, m_settings.viewDistance);
}

Game::Data Game::snapshot() {
  f32 aspect = static_cast<f32>(m_window.width()) / static_cast<f32>(m_window.height());
  mat4 projView = m_camera.proj(m_settings.fov, aspect) * m_camera.view();

  return Data{
    .guiSnapshot    = m_gui.snapshot(),
    .cameraProjView = projView,
    .cameraFrustrum = m_camera.frustrum(projView),
    .cameraPosition = m_camera.position(),
    .addedChunks    = std::move(m_world.addedChunks()),
    .removedChunks  = std::move(m_world.removedChunks())
  };
}

void Game::processInput(f32 dt) {
  static const f32 _speed = 10.0f;
  if (m_gui.ownsInput()) return;

  if (m_window.isPressed(MouseButton::Left)) {
    vec2 delta = m_window.cursorDelta();
    m_camera.rotate(-delta.y * m_settings.sensitivity, delta.x * m_settings.sensitivity);
  }

  vec3 inputDir = vec3(
    m_window.isPressed(Key::D) - m_window.isPressed(Key::A),
    m_window.isPressed(Key::Space) - m_window.isPressed(Key::LeftShift),
    m_window.isPressed(Key::W) - m_window.isPressed(Key::S)
  );
  if (inputDir != vec3(0.0f))
    m_camera.translate(glm::normalize(inputDir) * _speed * dt);
}

void Game::drawGUI() {
  m_gui.newFrame();
  m_profiler.draw();
}