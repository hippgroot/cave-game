#pragma once

#include "./chunk.hpp"

class World {
  using ChunkCache = std::unordered_map<ivec2, std::optional<Chunk>>;

  Channel<Chunk>&     m_chunkGenerated;
  JobPool&            m_jobPool;

  ChunkCache          m_chunkCache;

public:
  World(Channel<Chunk>& chunkGenerated, JobPool& jobPool);
  ~World() = default;

  World(const World&) = delete;
  World(World&&) = delete;
  World& operator=(const World&) = delete;
  World& operator=(World&&) = delete;

  Chunk& operator[](ivec2 coord);
  const Chunk& operator[](ivec2 coord) const;

private:
  static void job_generateChunk(Channel<Chunk>& chunkGenerated, ivec2 coord);
};