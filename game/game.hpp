#pragma once

#include "player/player.hpp"

class Game {
public:
  struct Input {
    vec3  worldUp;
    vec3  worldRight;
    vec3  dir;
    f32   pitch;
    f32   yaw;
  };

  struct WorldConfig {
    i64 seed;
    f32 surfaceFrequency;
    f32 surfaceLacunarity;
    f32 surfaceGain;
    i32 surfaceOctaves;
    f32 caveFrequency;
    f32 caveThreshold;
  };

private:
  Player m_player;

public:
  explicit Game(WorldConfig config);
  ~Game() = default;

  Game(const Game&) = delete;
  Game(Game&&) = delete;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&) = delete;

  void update(const Input& input);
  const Player& player() const;
};