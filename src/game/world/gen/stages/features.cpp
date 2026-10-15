#include "game/world/gen/stages/features.hpp"

#include "game/world/gen/rng.hpp"
#include "game/world/gen/shape.hpp"

#include <array>
#include <cmath>
#include <numbers>

namespace {

struct Ore {
  Block block;
  i32   veins;
  i32   size;
  i32   minY;
  i32   maxY;
};

constexpr std::array<Ore, 4> _ores{{
  {Block::CoalOre, 20, 16, 5, 128},
  {Block::IronOre, 20, 8, 5, 64},
  {Block::GoldOre, 2, 8, 5, 32},
  {Block::DiamondOre, 1, 7, 5, 16},
}};

constexpr i32 _minTrunk   = 4;
constexpr i32 _trunkRange = 3;
constexpr f32 _pi         = std::numbers::pi_v<f32>;

bool isOre(Block block) {
  return block == Block::CoalOre || block == Block::IronOre || block == Block::GoldOre || block == Block::DiamondOre;
}

bool replaces(Block block, Block existing) {
  switch (block) {
  case Block::Log:    return existing == Block::Air || existing == Block::Leaves;
  case Block::Leaves: return existing == Block::Air;
  default:            return existing == Block::Stone || (isOre(block) && isOre(existing) && existing < block);
  }
}

void place(Region& region, ivec3 world, Block block) {
  if (!region.contains(world)) return;
  if (replaces(block, region.get(world))) region.set(world, block);
}

void vein(Region& region, Rng& rng, const Ore& ore) {
  const ivec3 origin = region.origin();
  const vec3  at     = {
    static_cast<f32>(origin.x + rng.nextInt(Chunk::width)),
    static_cast<f32>(ore.minY + rng.nextInt(ore.maxY - ore.minY)),
    static_cast<f32>(origin.z + rng.nextInt(Chunk::width))
  };

  const f32  angle  = rng.nextFloat() * _pi;
  const f32  spread = static_cast<f32>(ore.size) / 8.0f;
  const vec3 offset = {std::sin(angle) * spread, 0.0f, std::cos(angle) * spread};
  const vec3 from   = at + offset + vec3(0.0f, static_cast<f32>(rng.nextInt(3) - 2), 0.0f);
  const vec3 to     = at - offset + vec3(0.0f, static_cast<f32>(rng.nextInt(3) - 2), 0.0f);

  for (i32 i = 0; i < ore.size; ++i) {
    const f32  t      = static_cast<f32>(i) / static_cast<f32>(ore.size);
    const vec3 center = glm::mix(from, to, t);
    const f32  blob   = rng.nextFloat() * static_cast<f32>(ore.size) / 16.0f;
    const f32  half   = ((std::sin(t * _pi) + 1.0f) * blob + 1.0f) / 2.0f;

    const ivec3 lo = ivec3(glm::floor(center - half));
    const ivec3 hi = ivec3(glm::floor(center + half));
    for (i32 x = lo.x; x <= hi.x; ++x) {
      for (i32 y = lo.y; y <= hi.y; ++y) {
        for (i32 z = lo.z; z <= hi.z; ++z) {
          const vec3 d = (vec3(x, y, z) + 0.5f - center) / half;
          if (glm::dot(d, d) < 1.0f) place(region, {x, y, z}, ore.block);
        }
      }
    }
  }
}

void tree(Region& region, Rng& rng, ivec3 ground) {
  const i32   trunk = _minTrunk + rng.nextInt(_trunkRange);
  const ivec3 base  = ground + ivec3(0, 1, 0);
  const i32   top   = base.y + trunk;
  if (top + 1 >= Chunk::height) return;

  region.set(ground, Block::Dirt);

  for (i32 y = top - 3; y <= top; ++y) {
    const i32 layer  = y - top;
    const i32 radius = layer >= -1 ? 1 : 2;

    for (i32 dz = -radius; dz <= radius; ++dz) {
      for (i32 dx = -radius; dx <= radius; ++dx) {
        const bool corner = std::abs(dx) == radius && std::abs(dz) == radius;
        if (corner && (layer == 0 || rng.nextInt(2) == 0)) continue;
        place(region, {base.x + dx, y, base.z + dz}, Block::Leaves);
      }
    }
  }

  for (i32 i = 0; i < trunk; ++i)
    place(region, base + ivec3(0, i, 0), Block::Log);
}

}

FeatureStage::FeatureStage(u64 seed)
: m_seed(deriveSeed(seed, name())) {}

void FeatureStage::generate(Region& region) const {
  ProtoChunk& proto  = region.self();
  const ivec3 origin = region.origin();
  Rng         rng(deriveSeed(m_seed, region.pos()));

  for (const Ore& ore : _ores)
    for (i32 i = 0; i < ore.veins; ++i)
      vein(region, rng, ore);

  const f32 density = biomeInfo(proto.chunk.biome(Chunk::width / 2, Chunk::width / 2)).trees;
  const i32 count   = static_cast<i32>(density) + (rng.chance(density - std::floor(density)) ? 1 : 0);

  for (i32 i = 0; i < count; ++i) {
    const i32 x      = rng.nextInt(Chunk::width);
    const i32 z      = rng.nextInt(Chunk::width);
    const i32 height = proto.heightmap[ProtoChunk::column(x, z)];
    if (height < seaLevel || height + 1 >= Chunk::height) continue;
    if (biomeInfo(proto.chunk.biome(x, z)).trees <= 0.0f) continue;

    const Block ground = proto.chunk.get({x, height, z});
    if (ground != Block::Grass && ground != Block::Snow) continue;
    if (proto.chunk.get({x, height + 1, z}) != Block::Air) continue;

    tree(region, rng, origin + ivec3(x, height, z));
  }
}
