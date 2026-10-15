#include "client/controller.hpp"

#include "game/player/player.hpp"

#include <grf/window.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr f32 _radiansPerPixel = 0.0025f;

}

Controls Controller::sample(const grf::Window& window, const Settings::Controls& settings, bool active) {
  if (active) {
    const vec2 motion = window.cursorMotion() * _radiansPerPixel * settings.sensitivity;
    m_yaw             = std::remainder(m_yaw - motion.x, 2.0f * std::numbers::pi_v<f32>);
    m_pitch           = std::clamp(m_pitch + (settings.invertY ? motion.y : -motion.y), -Player::maxPitch, Player::maxPitch);
  }

  Controls controls{.pitch = m_pitch, .yaw = m_yaw};
  if (!active) return controls;

  if (window.isPressed(grf::Key::W)) controls.move.y += 1.0f;
  if (window.isPressed(grf::Key::S)) controls.move.y -= 1.0f;
  if (window.isPressed(grf::Key::D)) controls.move.x += 1.0f;
  if (window.isPressed(grf::Key::A)) controls.move.x -= 1.0f;
  if (window.isPressed(grf::Key::Space)) controls.press(Button::Jump);
  if (window.isPressed(grf::Key::LeftControl)) controls.press(Button::Crouch);

  return controls;
}

void Controller::reset() {
  m_pitch = 0.0f;
  m_yaw   = 0.0f;
}
