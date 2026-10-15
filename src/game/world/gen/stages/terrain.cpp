#include "game/world/gen/stages/terrain.hpp"

#include "game/world/gen/shape.hpp"

#include <array>

namespace {

constexpr i32 _cellWidth  = 4;
constexpr i32 _cellHeight = 8;
constexpr i32 _cornersXZ  = Chunk::width / _cellWidth + 1;
constexpr i32 _cornersY   = Chunk::height / _cellHeight + 1;

constexpr f32 _frequency     = 0.012f;
constexpr i32 _octaves       = 3;
constexpr f32 _verticalScale = 1.5f;
constexpr i32 _floorSlide    = 8;
constexpr f32 _floorStrength = 4.0f;
constexpr i32 _ceilingSlide  = Chunk::height - 24;
constexpr f32 _ceilingFade   = 2.0f;

constexpr size corner(i32 x, i32 y, i32 z) {
  return static_cast<size>((y * _cornersXZ + z) * _cornersXZ + x);
}

}

TerrainStage::TerrainStage(u64 seed, std::shared_ptr<const Climate> climate)
: m_climate(std::move(climate)) {
  m_noise.SetSeed(noiseSeed(deriveSeed(seed, name())));
  m_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  m_noise.SetFrequency(_frequency);
  m_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  m_noise.SetFractalOctaves(_octaves);
}

void TerrainStage::generate(Region& region) const {
  ProtoChunk& proto  = region.self();
  const ivec3 origin = region.origin();

  std::array<f32, static_cast<size>(_cornersXZ * _cornersXZ * _cornersY)> density{};

  for (i32 cz = 0; cz < _cornersXZ; ++cz) {
    for (i32 cx = 0; cx < _cornersXZ; ++cx) {
      const i32          wx    = origin.x + cx * _cellWidth;
      const i32          wz    = origin.z + cz * _cellWidth;
      const TerrainShape shape = terrainShape(m_climate->sample(wx, wz));

      for (i32 cy = 0; cy < _cornersY; ++cy) {
        const i32 y     = cy * _cellHeight;
        const f32 noise = m_noise.GetNoise(static_cast<f32>(wx), static_cast<f32>(y) * _verticalScale, static_cast<f32>(wz));

        f32 value = shape.height - static_cast<f32>(y) + shape.roughness * noise;
        if (y < _floorSlide) value += static_cast<f32>(_floorSlide - y) * _floorStrength;
        if (y > _ceilingSlide) value -= static_cast<f32>(y - _ceilingSlide) * _ceilingFade;

        density[corner(cx, cy, cz)] = value;
      }
    }
  }

  proto.heightmap.fill(-1);

  for (i32 y = 0; y < Chunk::height; ++y) {
    const i32 cy = y / _cellHeight;
    const f32 ty = static_cast<f32>(y % _cellHeight) / _cellHeight;

    for (i32 z = 0; z < Chunk::width; ++z) {
      const i32 cz = z / _cellWidth;
      const f32 tz = static_cast<f32>(z % _cellWidth) / _cellWidth;

      for (i32 x = 0; x < Chunk::width; ++x) {
        const i32 cx = x / _cellWidth;
        const f32 tx = static_cast<f32>(x % _cellWidth) / _cellWidth;

        const auto at = [&](i32 dx, i32 dy, i32 dz) { return density[corner(cx + dx, cy + dy, cz + dz)]; };

        const f32 x00   = glm::mix(at(0, 0, 0), at(1, 0, 0), tx);
        const f32 x10   = glm::mix(at(0, 1, 0), at(1, 1, 0), tx);
        const f32 x01   = glm::mix(at(0, 0, 1), at(1, 0, 1), tx);
        const f32 x11   = glm::mix(at(0, 1, 1), at(1, 1, 1), tx);
        const f32 value = glm::mix(glm::mix(x00, x10, ty), glm::mix(x01, x11, ty), tz);

        if (value > 0.0f) {
          proto.chunk.set({x, y, z}, Block::Stone);
          proto.heightmap[ProtoChunk::column(x, z)] = static_cast<i16>(y);
        } else if (y <= seaLevel) {
          proto.chunk.set({x, y, z}, Block::Water);
        }
      }
    }
  }
}
