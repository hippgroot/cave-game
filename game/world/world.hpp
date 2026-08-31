#pragma once

#include "chunk.hpp"
#include "thread/channel.hpp"
#include "thread/job_pool.hpp"
#include "random.hpp"
#include "core/mesh.hpp"

#include <limits>

struct ivec2_hash {
  std::size_t operator()(ivec2 v) const {
    uvec2 uv = uvec2(v);
    u64 seed = (u64(uv.x) << 32) | u64(uv.y);
    return std::hash<u64>{}(seed);
  }
};

class World {
public:
  using GenRes = std::pair<Chunk, MeshData>;

private:
  using Chunks = std::unordered_map<ivec2, std::optional<Chunk>, ivec2_hash>;

  Random&               m_random;
  Channel<GenRes>&      m_chunkGenerated;
  JobPool&              m_jobPool;

  ivec2                 m_currChunk = ivec2(std::numeric_limits<i32>::max(), std::numeric_limits<i32>::max());
  Chunks                m_chunks;
  std::vector<MeshData> m_addedChunks;
  std::vector<u32>      m_removedChunks;

public:
  World(Random& rand, Channel<GenRes>& chunkGenerated, JobPool& jobPool);
  ~World() = default;

  World(const World&) = delete;
  World(World&&) = delete;
  World& operator=(const World&) = delete;
  World& operator=(World&&) = delete;

  std::optional<Chunk>& operator[](ivec2 coord);
  const std::optional<Chunk>& operator[](ivec2 coord) const;

  static ivec2 getChunkCoord(vec3 position);
  static vec3 getWorldPosition(ivec2 coord);

  void updateChunks(ivec2 currChunk, i32 viewDistance);
  std::vector<MeshData> addedChunks();
  std::vector<u32> removedChunks();

private:
  static void job_generateChunk(Random& random, Channel<GenRes>& chunkGenerated, ivec2 coord);
  static std::vector<Block> generateBlocks(Random& random, ivec2 coord);
  static Chunk::Mesh generateMesh(const std::vector<Block>& blocks);

  void unloadChunks(i32 unloadDistance);
  void receiveChunks();
  void requestChunks(i32 viewDistance);
};