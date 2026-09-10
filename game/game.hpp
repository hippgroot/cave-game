#pragma once

#include "player/player.hpp"
#include "world/world.hpp"

#include <variant>

class Game {
public:
  struct Input {
    vec3  worldUp;
    vec3  worldRight;
    vec3  dir;
    f32   pitch;
    f32   yaw;
    i32   viewDistance;
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

  struct ChunkLoadEvent {
    Chunk chunk;
  };

  struct ChunkUnloadEvent {
    ivec2 coord;
  };

  using Event = std::variant<ChunkLoadEvent, ChunkUnloadEvent>;

private:
  Random&             m_rng;
  JobPool&            m_jobPool;
  World               m_world;
  Player              m_player;
  std::vector<Event>  m_events;

public:
  explicit Game(Random& rng, JobPool& jobPool, Channel<Chunk>& chunkGenerated, World::Config config);
  ~Game() = default;

  Game(const Game&) = delete;
  Game(Game&&) = delete;
  Game& operator=(const Game&) = delete;
  Game& operator=(Game&&) = delete;

  void update(const Input& input);
  const Player& player() const;
  std::vector<Event> events();
};