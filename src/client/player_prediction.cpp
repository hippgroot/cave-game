#include "client/player_prediction.hpp"

#include "game/game.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr u32  _maxSteps        = 5;
constexpr size _maxPending      = Game::tickRate * 2;
constexpr f32  _correctionDecay = 12.0f;
constexpr f32  _snapDistance    = 8.0f;

PlayerMode modeOf(u8 mode, PlayerMode fallback) {
  return mode < static_cast<u8>(PlayerMode::Count) ? static_cast<PlayerMode>(mode) : fallback;
}

}

PlayerPrediction::PlayerPrediction(const protocol::PlayerSync& sync)
: m_player(Player::Info{.position = sync.position, .mode = modeOf(sync.mode, PlayerMode::Spectator)}),
  m_previous(m_player),
  m_acked(sync.lastInputSequence),
  m_nextSequence(sync.lastInputSequence + 1) {
  m_player.place(sync.position, sync.velocity);
  m_previous = m_player;
}

std::vector<protocol::PlayerInput> PlayerPrediction::advance(f32 dt, const Controls& controls) {
  m_latest = controls;
  m_correction *= std::exp(-_correctionDecay * dt);
  m_accumulator = std::min(m_accumulator + dt, Game::tickDelta * _maxSteps);

  std::vector<protocol::PlayerInput> inputs;
  while (m_accumulator >= Game::tickDelta) {
    m_accumulator -= Game::tickDelta;

    const u32 sequence = m_nextSequence++;
    m_previous         = m_player;
    m_player.step(controls, Game::tickDelta);

    m_pending.push_back(Pending{.sequence = sequence, .controls = controls});
    inputs.push_back(controls.toInput(sequence));
  }

  while (m_pending.size() > _maxPending)
    m_pending.pop_front();

  return inputs;
}

void PlayerPrediction::reconcile(const protocol::PlayerSync& sync) {
  if (sync.lastInputSequence < m_acked) return;
  m_acked = sync.lastInputSequence;

  while (!m_pending.empty() && m_pending.front().sequence <= m_acked)
    m_pending.pop_front();

  const vec3 before = position();

  Player current = m_player;
  current.setMode(modeOf(sync.mode, current.mode()));
  current.place(sync.position, sync.velocity);

  Player previous = current;
  for (const Pending& pending : m_pending) {
    previous = current;
    current.step(pending.controls, Game::tickDelta);
  }

  m_player = current;
  if (!m_pending.empty()) m_previous = previous;

  m_correction = before - interpolated();
  if (glm::length(m_correction) > _snapDistance) m_correction = vec3(0.0f);
}

const Player& PlayerPrediction::player() const {
  return m_player;
}

vec3 PlayerPrediction::position() const {
  return interpolated() + m_correction;
}

vec3 PlayerPrediction::eye() const {
  return position() + vec3(0.0f, Player::eyeHeight, 0.0f);
}

f32 PlayerPrediction::pitch() const {
  return m_latest.pitch;
}

f32 PlayerPrediction::yaw() const {
  return m_latest.yaw;
}

size PlayerPrediction::pending() const {
  return m_pending.size();
}

vec3 PlayerPrediction::correction() const {
  return m_correction;
}

vec3 PlayerPrediction::interpolated() const {
  return glm::mix(m_previous.position(), m_player.position(), m_accumulator / Game::tickDelta);
}
