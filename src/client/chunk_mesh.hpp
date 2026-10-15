#pragma once

#include "engine/mesh.hpp"
#include "game/world/chunk.hpp"

#include <array>
#include <vector>

struct ChunkVertex {
  vec3 position;
  vec2 uv;
  vec3 normal;
};

static_assert(sizeof(ChunkVertex) == 32);

struct ChunkGeometry {
  std::vector<ChunkVertex> vertices;
  std::vector<u32>         indices;
  std::vector<u32>         blocks;
  i32                      minY = Chunk::height;
  i32                      maxY = -1;

  bool empty() const { return indices.empty(); }
};

struct ChunkMeshes {
  ChunkGeometry opaque;
  ChunkGeometry translucent;
};

enum class Side : u8 {
  PosX,
  NegX,
  PosZ,
  NegZ
};

using ChunkNeighbors = std::array<const Chunk*, 4>;

ChunkMeshes buildChunkMeshes(const Chunk& chunk, const ChunkNeighbors& neighbors);
MeshData    toMeshData(ChunkPos pos, ChunkGeometry geometry, bool translucent);
