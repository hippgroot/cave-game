#pragma once

#include "core.hpp"
#include <limits>

enum class Block : u8 {
  Air,
  Grass,
  Dirt,
  Stone,
  Bedrock,
  Invalid = std::numeric_limits<u8>::max()
};

class Chunk {
public:
  struct Vertex {
    vec3 pos;
    vec2 uv;
    vec3 norm;
  };

  struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<u32>    indices;
    std::vector<u32>    types;
  };

private:
  static const i32 m_size = 16;
  static const i32 m_height = 256;

  ivec2               m_coord;
  std::vector<Block>  m_blocks;

public:
  Chunk(ivec2 coord, std::vector<Block> blocks);
  ~Chunk() = default;

  Chunk(const Chunk&) = default;
  Chunk(Chunk&&) = default;
  Chunk& operator=(const Chunk&) = default;
  Chunk& operator=(Chunk&&) = default;

  static i32 size();
  static i32 height();

  const ivec2& coord() const;
};

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