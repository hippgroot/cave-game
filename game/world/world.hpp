#pragma once

#include "./chunk.hpp"

struct ivec2_hash {
  std::size_t operator()(ivec2 v) const {
    i64 seed = (static_cast<i64>(v.x) << 32) | static_cast<i64>(v.y);
    return std::hash<i64>{}(seed);
  }
};

class World {
public:
  struct Config {
    i64 seed;
    f32 surfaceFrequency;
    f32 surfaceLacunarity;
    f32 surfaceGain;
    i32 surfaceOctaves;
    f32 caveFrequency;
    f32 caveThreshold;
  };

private:
  using ChunkCache = std::unordered_map<ivec2, std::optional<Chunk>, ivec2_hash>;

  Random&               m_rng;
  Channel<Chunk>&       m_chunkGenerated;
  JobPool&              m_jobPool;

  Config                m_config;
  ChunkCache            m_chunkCache;
  std::optional<ivec2>  m_prevCenter;
  std::vector<ivec2>    m_addedChunks;
  std::vector<ivec2>    m_removedChunks;

public:
  World(Random& rng, Channel<Chunk>& chunkGenerated, JobPool& jobPool, Config config);
  ~World() = default;

  World(const World&) = delete;
  World(World&&) = delete;
  World& operator=(const World&) = delete;
  World& operator=(World&&) = delete;

  const std::optional<Chunk>& operator[](ivec2 coord) const;

  void update(ivec2 chunkCenter, i32 viewDistance);
  std::vector<ivec2> addedChunks();
  std::vector<ivec2> removedChunks();

private:
  void unloadChunks(ivec2 center, i32 unloadDistance);
  void receiveChunks();
  void requestChunks(ivec2 center, i32 viewDistance);

  static void job_generateChunk(Random& rng, Channel<Chunk>& chunkGenerated, ivec2 coord, Config config);
};