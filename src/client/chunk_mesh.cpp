#include "client/chunk_mesh.hpp"

#include <algorithm>
#include <span>

namespace {

constexpr f32 _tiles       = 3.0f;
constexpr f32 _waterHeight = 0.875f;
constexpr u8  _opaqueLayer = 0;
constexpr u8  _clearLayer  = 8;

constexpr std::array<ivec3, 6> _offsets{{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
constexpr std::array<f32, 6>   _faceTile{1.0f, 1.0f, 0.0f, 2.0f, 1.0f, 1.0f};
constexpr size                 _top    = 2;
constexpr size                 _bottom = 3;

constexpr std::array<std::array<vec3, 4>, 6> _corners{{
  {{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}},
  {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}},
  {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}},
  {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}},
  {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},
  {{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}},
}};

constexpr std::array<std::array<vec2, 4>, 6> _uvs{{
  {{{1, 1}, {1, 0}, {0, 0}, {0, 1}}},
  {{{0, 1}, {1, 1}, {1, 0}, {0, 0}}},
  {{{0, 0}, {0, 1}, {1, 1}, {1, 0}}},
  {{{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
  {{{0, 1}, {1, 1}, {1, 0}, {0, 0}}},
  {{{1, 1}, {1, 0}, {0, 0}, {0, 1}}},
}};

constexpr std::array<u32, 6> _quad{0, 1, 2, 2, 3, 0};

class Sampler {
  const Chunk&          m_chunk;
  const ChunkNeighbors& m_neighbors;

public:
  Sampler(const Chunk& chunk, const ChunkNeighbors& neighbors)
  : m_chunk(chunk),
    m_neighbors(neighbors) {}

  Block at(ivec3 local) const {
    if (local.y < 0) return Block::Bedrock;
    if (local.y >= Chunk::height) return Block::Air;

    if (local.x >= Chunk::width) return from(Side::PosX, local - ivec3(Chunk::width, 0, 0));
    if (local.x < 0) return from(Side::NegX, local + ivec3(Chunk::width, 0, 0));
    if (local.z >= Chunk::width) return from(Side::PosZ, local - ivec3(0, 0, Chunk::width));
    if (local.z < 0) return from(Side::NegZ, local + ivec3(0, 0, Chunk::width));

    return m_chunk.get(local);
  }

private:
  Block from(Side side, ivec3 local) const {
    const Chunk* neighbor = m_neighbors[static_cast<size>(side)];
    return neighbor == nullptr ? Block::Air : neighbor->get(local);
  }
};

bool visible(Block block, Block neighbor, size face) {
  if (block == Block::Water) return face == _top ? neighbor != Block::Water : neighbor == Block::Air;
  return neighbor == Block::Air || neighbor == Block::Water;
}

void emit(ChunkGeometry& geometry, Block block, ivec3 local, size face, f32 height) {
  const u32 base = static_cast<u32>(geometry.vertices.size());

  for (size corner = 0; corner < 4; ++corner) {
    vec3 position = _corners[face][corner];
    if (position.y > 0.0f) position.y = height;

    const vec2 uv = _uvs[face][corner];
    geometry.vertices.push_back(ChunkVertex{
      .position = vec3(local) + position,
      .uv       = {(_faceTile[face] + uv.x) / _tiles, uv.y},
      .normal   = vec3(_offsets[face])
    });
  }

  for (const u32 index : _quad)
    geometry.indices.push_back(base + index);

  geometry.blocks.push_back(static_cast<u32>(block));
  geometry.minY = std::min(geometry.minY, local.y);
  geometry.maxY = std::max(geometry.maxY, local.y);
}

}

ChunkMeshes buildChunkMeshes(const Chunk& chunk, const ChunkNeighbors& neighbors) {
  ChunkMeshes   meshes;
  const Sampler sampler(chunk, neighbors);

  for (i32 y = 0; y < Chunk::height; ++y) {
    for (i32 z = 0; z < Chunk::width; ++z) {
      for (i32 x = 0; x < Chunk::width; ++x) {
        const ivec3 local = {x, y, z};
        const Block block = chunk.get(local);
        if (block == Block::Air) continue;

        const bool     water    = block == Block::Water;
        ChunkGeometry& geometry = water ? meshes.translucent : meshes.opaque;
        const f32      height   = water && sampler.at(local + ivec3(0, 1, 0)) != Block::Water ? _waterHeight : 1.0f;

        for (size face = 0; face < _offsets.size(); ++face) {
          if (face == _bottom && y == 0) continue;

          if (!visible(block, sampler.at(local + _offsets[face]), face)) continue;

          emit(geometry, block, local, face, height);
        }
      }
    }
  }

  return meshes;
}

MeshData toMeshData(ChunkPos pos, ChunkGeometry geometry, bool translucent) {
  const auto vertexBytes = std::as_bytes(std::span(geometry.vertices));
  const auto blockBytes  = std::as_bytes(std::span(geometry.blocks));

  const std::array<i32, 3> constants{pos.x, pos.y, Chunk::width};
  const auto               constantBytes = std::as_bytes(std::span(constants));

  const vec3 origin = vec3(Chunk::origin(pos));

  return MeshData{
    .vertices      = std::vector<byte>(vertexBytes.begin(), vertexBytes.end()),
    .indices       = std::move(geometry.indices),
    .aux           = std::vector<byte>(blockBytes.begin(), blockBytes.end()),
    .pushConstants = std::vector<byte>(constantBytes.begin(), constantBytes.end()),
    .aabb          = AABB{
      .min = origin + vec3(0.0f, static_cast<f32>(geometry.minY), 0.0f),
      .max = origin + vec3(Chunk::width, static_cast<f32>(geometry.maxY + 1), Chunk::width)
    },
    .pipeline = translucent ? "chunk_translucent" : "chunk_draw",
    .meshID   = 0,
    .layer    = translucent ? _clearLayer : _opaqueLayer
  };
}
