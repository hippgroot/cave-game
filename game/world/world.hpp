#pragma once

#include "chunk.hpp"

#include <limits>

class World {
public:
  using AddedChunks = std::vector<std::pair<ivec2, Chunk::Mesh>>;
  using GenRes = std::pair<Chunk, Chunk::Mesh>;

private:
  using Chunks = std::unordered_map<ivec2, std::optional<Chunk>, ivec2_hash>;

  Random&             m_random;
  Channel<GenRes>&    m_chunkGenerated;
  JobPool&            m_jobPool;

  ivec2               m_currChunk = ivec2(std::numeric_limits<i32>::max(), std::numeric_limits<i32>::max());
  Chunks              m_chunks;
  AddedChunks         m_addedChunks;
  std::vector<ivec2>  m_removedChunks;

public:
  World(Random& rand, Channel<GenRes>& chunkGenerated, JobPool& jobPool);
  ~World() = default;

  World(const World&) = delete;
  World(World&&) = delete;
  World& operator=(const World&) = delete;
  World& operator=(World&&) = delete;

  std::optional<Chunk>& operator[](ivec2 coord);
  const std::optional<Chunk>& operator[](ivec2 coord) const;

  void updateChunks(ivec2 currChunk, i32 viewDistance);
  AddedChunks addedChunks();
  std::vector<ivec2> removedChunks();

private:
  static void job_generateChunk(Random& random, Channel<GenRes>& chunkGenerated, ivec2 coord);
  static std::vector<Block> generateBlocks(Random& random, ivec2 coord);
  static Chunk::Mesh generateMesh(const std::vector<Block>& blocks);

  void unloadChunks(i32 unloadDistance);
  void receiveChunks();
  void requestChunks(i32 viewDistance);
};