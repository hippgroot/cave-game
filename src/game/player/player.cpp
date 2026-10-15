#include "game/player/player.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

f32 finite(f32 value) {
  return std::isfinite(value) ? value : 0.0f;
}

Controls sanitize(const Controls& controls) {
  Controls result = controls;
  result.move     = glm::clamp(vec2(finite(controls.move.x), finite(controls.move.y)), vec2(-1.0f), vec2(1.0f));
  result.pitch    = std::clamp(finite(controls.pitch), -Player::maxPitch, Player::maxPitch);
  result.yaw      = std::remainder(finite(controls.yaw), 2.0f * std::numbers::pi_v<f32>);
  return result;
}

static_assert(PlayerState<SpectatorState>);

}

std::string_view toString(PlayerMode mode) {
  switch (mode) {
  case PlayerMode::Spectator: return "Spectator";
  case PlayerMode::Count:     break;
  }
  return "Unknown";
}

Player::Player(const Info& info)
: m_body{.position = info.position},
  m_state(makeState(info.mode)) {
  std::visit([this](const auto& state) { state.enter(m_body); }, m_state);
}

void Player::step(const Controls& controls, f32 dt) {
  const Controls safe = sanitize(controls);
  m_body.pitch        = safe.pitch;
  m_body.yaw          = safe.yaw;

  std::visit([&](const auto& state) { state.step(m_body, safe, dt); }, m_state);

  m_body.position = glm::clamp(m_body.position, vec3(-worldBorder, minY, -worldBorder), vec3(worldBorder, maxY, worldBorder));
}

PlayerMode Player::mode() const {
  return std::visit([](const auto& state) { return std::remove_cvref_t<decltype(state)>::mode; }, m_state);
}

void Player::setMode(PlayerMode mode) {
  if (mode == this->mode()) return;

  std::visit([this](const auto& state) { state.exit(m_body); }, m_state);
  m_state = makeState(mode);
  std::visit([this](const auto& state) { state.enter(m_body); }, m_state);
}

const PlayerBody& Player::body() const {
  return m_body;
}

vec3 Player::position() const {
  return m_body.position;
}

vec3 Player::eye() const {
  return m_body.position + vec3(0.0f, eyeHeight, 0.0f);
}

void Player::place(vec3 position, vec3 velocity) {
  m_body.position = position;
  m_body.velocity = velocity;
}

Player::State Player::makeState(PlayerMode mode) {
  switch (mode) {
  case PlayerMode::Spectator:
  case PlayerMode::Count:     return SpectatorState{};
  }
  return SpectatorState{};
}
