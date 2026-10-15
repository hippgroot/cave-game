#include "game/world/gen/stages/carvers.hpp"

#include "game/world/gen/rng.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr i32 _range       = 8;
constexpr i32 _rarity      = 7;
constexpr i32 _maxStartY   = 120;
constexpr i32 _topMargin   = 8;
constexpr f32 _floorCutoff = -0.7f;
constexpr f32 _pi          = std::numbers::pi_v<f32>;

bool carvable(Block block) {
  switch (block) {
  case Block::Stone:
  case Block::Dirt:
  case Block::Grass:
  case Block::Sand:
  case Block::Sandstone:
  case Block::Gravel:
  case Block::Snow:      return true;
  default:               return false;
  }
}

class Carver {
  Chunk&   m_chunk;
  ChunkPos m_target;
  ivec3    m_origin;

public:
  Carver(Chunk& chunk, ChunkPos target)
  : m_chunk(chunk),
    m_target(target),
    m_origin(Chunk::origin(target)) {}

  void cave(Rng& rng, ChunkPos source) {
    i32 count = rng.nextInt(rng.nextInt(rng.nextInt(15) + 1) + 1);
    if (rng.nextInt(_rarity) != 0) count = 0;

    for (i32 i = 0; i < count; ++i) {
      const ivec3 base  = Chunk::origin(source);
      const vec3  start = {
        static_cast<f32>(base.x + rng.nextInt(Chunk::width)),
        static_cast<f32>(rng.nextInt(rng.nextInt(_maxStartY) + 8)),
        static_cast<f32>(base.z + rng.nextInt(Chunk::width))
      };

      i32 tunnels = 1;
      if (rng.nextInt(4) == 0) {
        tunnel(rng.next(), start, 1.0f + rng.nextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5f);
        tunnels += rng.nextInt(4);
      }

      for (i32 j = 0; j < tunnels; ++j) {
        const f32 yaw   = rng.nextFloat() * 2.0f * _pi;
        const f32 pitch = (rng.nextFloat() - 0.5f) * 2.0f / 8.0f;
        f32       width = rng.nextFloat() * 2.0f + rng.nextFloat();
        if (rng.nextInt(10) == 0) width *= rng.nextFloat() * rng.nextFloat() * 3.0f + 1.0f;

        tunnel(rng.next(), start, width, yaw, pitch, 0, 0, 1.0f);
      }
    }
  }

private:
  void tunnel(u64 seed, vec3 pos, f32 width, f32 yaw, f32 pitch, i32 step, i32 length, f32 yScale) {
    Rng rng(seed);

    const f32 centerX = static_cast<f32>(m_origin.x + Chunk::width / 2);
    const f32 centerZ = static_cast<f32>(m_origin.z + Chunk::width / 2);

    f32 yawSpeed   = 0.0f;
    f32 pitchSpeed = 0.0f;

    if (length <= 0) {
      length = _range * Chunk::width - Chunk::width;
      length -= rng.nextInt(length / 4);
    }

    const bool room = step == -1;
    if (room) step = length / 2;

    const i32  branch = rng.nextInt(length / 2) + length / 4;
    const bool steep  = rng.nextInt(6) == 0;

    for (; step < length; ++step) {
      const f32 radius  = 1.5f + std::sin(static_cast<f32>(step) * _pi / static_cast<f32>(length)) * width;
      const f32 yRadius = radius * yScale;

      const f32 horizontal = std::cos(pitch);
      pos += vec3(std::cos(yaw) * horizontal, std::sin(pitch), std::sin(yaw) * horizontal);

      pitch *= steep ? 0.92f : 0.7f;
      pitch += pitchSpeed * 0.1f;
      yaw += yawSpeed * 0.1f;
      pitchSpeed *= 0.9f;
      yawSpeed *= 0.75f;
      pitchSpeed += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
      yawSpeed += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

      if (!room && step == branch && width > 1.0f) {
        tunnel(rng.next(), pos, rng.nextFloat() * 0.5f + 0.5f, yaw - _pi / 2.0f, pitch / 3.0f, step, length, 1.0f);
        tunnel(rng.next(), pos, rng.nextFloat() * 0.5f + 0.5f, yaw + _pi / 2.0f, pitch / 3.0f, step, length, 1.0f);
        return;
      }

      if (!room && rng.nextInt(4) == 0) continue;

      const f32 dx        = pos.x - centerX;
      const f32 dz        = pos.z - centerZ;
      const f32 remaining = static_cast<f32>(length - step);
      const f32 reach     = width + 2.0f + Chunk::width;
      if (dx * dx + dz * dz - remaining * remaining > reach * reach) return;

      const f32 margin = Chunk::width + radius * 2.0f;
      if (dx < -margin || dz < -margin || dx > margin || dz > margin) continue;

      carve(pos, radius, yRadius);
      if (room) break;
    }
  }

  void carve(vec3 center, f32 radius, f32 yRadius) {
    const vec3  extent = {radius, yRadius, radius};
    const ivec3 lo     = glm::max(ivec3(glm::floor(center - extent)) - m_origin, ivec3(0, 1, 0));
    const ivec3 hi     = glm::min(ivec3(glm::floor(center + extent)) - m_origin + 1, ivec3(Chunk::width, Chunk::height - _topMargin, Chunk::width));
    if (lo.x >= hi.x || lo.y >= hi.y || lo.z >= hi.z) return;

    for (i32 x = lo.x; x < hi.x; ++x)
      for (i32 z = lo.z; z < hi.z; ++z)
        for (i32 y = lo.y - 1; y <= hi.y; ++y)
          if (Chunk::contains({x, y, z}) && m_chunk.get({x, y, z}) == Block::Water) return;

    for (i32 y = lo.y; y < hi.y; ++y) {
      const f32 ny = (static_cast<f32>(y) + 0.5f - center.y) / yRadius;
      if (ny <= _floorCutoff) continue;

      for (i32 z = lo.z; z < hi.z; ++z) {
        const f32 nz = (static_cast<f32>(m_origin.z + z) + 0.5f - center.z) / radius;

        for (i32 x = lo.x; x < hi.x; ++x) {
          const f32 nx = (static_cast<f32>(m_origin.x + x) + 0.5f - center.x) / radius;
          if (nx * nx + ny * ny + nz * nz >= 1.0f) continue;
          if (carvable(m_chunk.get({x, y, z}))) m_chunk.set({x, y, z}, Block::Air);
        }
      }
    }
  }
};

}

CarverStage::CarverStage(u64 seed)
: m_seed(deriveSeed(seed, name())) {}

void CarverStage::generate(Region& region) const {
  Carver carver(region.self().chunk, region.pos());

  for (i32 z = -_range; z <= _range; ++z) {
    for (i32 x = -_range; x <= _range; ++x) {
      const ChunkPos source = region.pos() + ivec2(x, z);
      Rng            rng(deriveSeed(m_seed, source));
      carver.cave(rng, source);
    }
  }
}
