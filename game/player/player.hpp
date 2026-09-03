#pragma once

#include "engine/core.hpp"

class Player {
  static constexpr f32 m_pitchLimit = glm::radians(75.0f);

  vec3  m_position = vec3(0.0f);
  f32   m_pitch = 0.0f;
  f32   m_yaw = 0.0f;
  f32   m_speed = 10.0f;

public:
  Player(vec3 position = vec3(0.0f), f32 pitch = 0.0f, f32 yaw = 0.0f);
  ~Player() = default;

  Player(const Player&) = default;
  Player(Player&&) = default;
  Player& operator=(const Player&) = default;
  Player& operator=(Player&&) = default;

  void translate(vec3 worldUp, vec3 worldRight, vec3 dir);
  void rotate(f32 pitch, f32 yaw);

  vec3 position() const;
  f32 pitch() const;
  f32 yaw() const;
};