#include "game/world/gen/stages/surface.hpp"

#include "game/world/gen/rng.hpp"
#include "game/world/gen/shape.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace {

constexpr i32 _steepSlope    = 4;
constexpr i32 _minDepth      = 3;
constexpr f32 _depthRange    = 1.5f;
constexpr f32 _depthFreq     = 0.05f;
constexpr i32 _baseDepth     = 3;
constexpr i32 _bedrockLayers = 5;

constexpr std::array<ivec2, 4> _sides{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

i32 heightAt(const Region& region, ivec2 world) {
  const ivec3       block = {world.x, 0, world.y};
  const ivec3       local = Chunk::localOf(block);
  const ProtoChunk& proto = region.read(Chunk::posOf(block) - region.pos());
  return proto.heightmap[ProtoChunk::column(local.x, local.z)];
}

}

SurfaceStage::SurfaceStage(u64 seed)
: m_seed(deriveSeed(seed, name())) {
  m_depthNoise.SetSeed(noiseSeed(m_seed));
  m_depthNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  m_depthNoise.SetFrequency(_depthFreq);
}

void SurfaceStage::generate(Region& region) const {
  ProtoChunk& proto  = region.self();
  Chunk&      chunk  = proto.chunk;
  const ivec3 origin = region.origin();
  Rng         rng(deriveSeed(m_seed, region.pos()));

  for (i32 z = 0; z < Chunk::width; ++z) {
    for (i32 x = 0; x < Chunk::width; ++x) {
      const BiomeInfo& biome  = biomeInfo(chunk.biome(x, z));
      const ivec2      world  = {origin.x + x, origin.z + z};
      const i32        height = proto.heightmap[ProtoChunk::column(x, z)];

      i32 slope = 0;
      for (const ivec2 side : _sides)
        slope = std::max(slope, std::abs(height - heightAt(region, world + side)));
      const bool steep = slope >= _steepSlope;

      const f32 noise  = m_depthNoise.GetNoise(static_cast<f32>(world.x), static_cast<f32>(world.y));
      const i32 filler = std::max(_minDepth, _minDepth + static_cast<i32>((noise + 1.0f) * _depthRange));

      i32   depth = -1;
      Block above = Block::Air;

      for (i32 y = std::min(height + 1, Chunk::height - 1); y > 0; --y) {
        const Block block = chunk.get({x, y, z});
        if (block == Block::Air || block == Block::Water) {
          depth = -1;
          above = block;
          continue;
        }
        if (block != Block::Stone) continue;

        ++depth;
        const bool wet = above == Block::Water;

        Block replacement = Block::Stone;
        if (depth == 0) {
          if (wet)
            replacement = biome.underwater;
          else if (y >= snowLine)
            replacement = Block::Snow;
          else if (steep)
            replacement = biome.snowy ? Block::Snow : Block::Stone;
          else
            replacement = biome.top;
        } else if (depth <= filler) {
          replacement = wet ? biome.underwater : (steep ? Block::Stone : biome.filler);
        } else if (depth <= filler + _baseDepth && !steep) {
          replacement = biome.base;
        } else {
          continue;
        }

        chunk.set({x, y, z}, replacement);
      }

      chunk.set({x, 0, z}, Block::Bedrock);
      for (i32 y = 1; y < _bedrockLayers; ++y)
        if (y <= rng.nextInt(_bedrockLayers)) chunk.set({x, y, z}, Block::Bedrock);
    }
  }
}
