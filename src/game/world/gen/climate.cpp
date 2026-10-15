#include "game/world/gen/climate.hpp"

#include "game/world/gen/stage.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace {

constexpr f32 _scale           = 1.6f;
constexpr f32 _continentalBias = 0.1f;

FastNoiseLite makeNoise(u64 seed, std::string_view name, f32 frequency, i32 octaves) {
  FastNoiseLite noise;
  noise.SetSeed(noiseSeed(deriveSeed(seed, name)));
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  noise.SetFrequency(frequency);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(octaves);
  return noise;
}

f32 sampleNoise(const FastNoiseLite& noise, f32 x, f32 z) {
  return std::clamp(noise.GetNoise(x, z) * _scale, -1.0f, 1.0f);
}

}

Climate::Climate(u64 seed)
: m_temperature(makeNoise(seed, "temperature", 0.0009f, 3)),
  m_humidity(makeNoise(seed, "humidity", 0.0011f, 3)),
  m_continentalness(makeNoise(seed, "continentalness", 0.0010f, 5)),
  m_erosion(makeNoise(seed, "erosion", 0.0020f, 4)),
  m_weirdness(makeNoise(seed, "weirdness", 0.0035f, 3)) {}

ClimatePoint Climate::sample(i32 x, i32 z) const {
  const f32 fx = static_cast<f32>(x);
  const f32 fz = static_cast<f32>(z);

  ClimatePoint point{
    .temperature     = sampleNoise(m_temperature, fx, fz),
    .humidity        = sampleNoise(m_humidity, fx, fz),
    .continentalness = std::clamp(sampleNoise(m_continentalness, fx, fz) + _continentalBias, -1.0f, 1.0f),
    .erosion         = sampleNoise(m_erosion, fx, fz),
    .weirdness       = sampleNoise(m_weirdness, fx, fz),
  };
  point.peaksValleys = -3.0f * (std::abs(std::abs(point.weirdness) - 2.0f / 3.0f) - 1.0f / 3.0f);
  return point;
}
