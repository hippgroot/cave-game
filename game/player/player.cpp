#include "./player.hpp"

Player::Player(vec3 position, f32 pitch, f32 yaw)
: m_position(position),
  m_pitch(pitch),
  m_yaw(yaw)
{}

void Player::translate(vec3 worldUp, vec3 worldRight, vec3 dir) {
  vec3 delta = m_speed * dir;
  quat rotation = glm::angleAxis(m_yaw, worldUp) * glm::angleAxis(m_pitch, worldRight);

  m_position += rotation * vec3(delta.x, 0.0f, delta.z);
  m_position.y += delta.y;
}

void Player::rotate(f32 pitch, f32 yaw) {
  m_pitch = glm::clamp(m_pitch + pitch, -m_pitchLimit, m_pitchLimit);
  m_yaw += yaw;
}

vec3 Player::position() const {
  return m_position;
}

f32 Player::pitch() const {
  return m_pitch;
}

f32 Player::yaw() const {
  return m_yaw;
}