#pragma once

#include "core/types.hpp"
#include "game/world/biome.hpp"
#include "game/world/block.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <optional>
#include <span>
#include <vector>

using ChunkPos = ivec2;

struct ChunkPosHash {
  size operator()(ChunkPos pos) const {
    u64 key = (static_cast<u64>(static_cast<u32>(pos.x)) << 32) | static_cast<u32>(pos.y);
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccdull;
    key ^= key >> 33;
    return static_cast<size>(key);
  }
};

class Chunk {
public:
  static constexpr i32 width  = 16;
  static constexpr i32 height = 256;
  static constexpr i32 area   = width * width;
  static constexpr i32 volume = area * height;

private:
  std::vector<Block>      m_blocks;
  std::array<Biome, area> m_biomes{};

public:
  Chunk();
  ~Chunk() = default;

  Chunk(const Chunk&)            = default;
  Chunk(Chunk&&)                 = default;
  Chunk& operator=(const Chunk&) = default;
  Chunk& operator=(Chunk&&)      = default;

  static constexpr bool contains(ivec3 local) {
    return local.x >= 0 && local.x < width && local.y >= 0 && local.y < height && local.z >= 0 && local.z < width;
  }

  static constexpr i32 index(ivec3 local) { return (local.y * width + local.z) * width + local.x; }
  static constexpr i32 column(i32 x, i32 z) { return z * width + x; }

  static constexpr i32 floorDiv(i32 value) { return value >= 0 ? value / width : (value - width + 1) / width; }
  static constexpr i32 floorMod(i32 value) { return value - floorDiv(value) * width; }

  static constexpr ChunkPos posOf(ivec3 world) { return {floorDiv(world.x), floorDiv(world.z)}; }
  static constexpr ivec3    localOf(ivec3 world) { return {floorMod(world.x), world.y, floorMod(world.z)}; }
  static constexpr ivec3    origin(ChunkPos pos) { return {pos.x * width, 0, pos.y * width}; }

  static i32 distance(ChunkPos a, ChunkPos b) { return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)); }

  Block get(ivec3 local) const { return m_blocks[index(local)]; }
  void  set(ivec3 local, Block block) { m_blocks[index(local)] = block; }

  Biome biome(i32 x, i32 z) const { return m_biomes[column(x, z)]; }
  void  setBiome(i32 x, i32 z, Biome biome) { m_biomes[column(x, z)] = biome; }

  std::span<const Block> blocks() const { return m_blocks; }

  std::vector<u8>             encode() const;
  static std::optional<Chunk> decode(std::span<const u8> bytes);
};
