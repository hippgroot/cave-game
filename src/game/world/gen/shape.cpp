#include "game/world/gen/shape.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace {

constexpr std::array<vec2, 9> _continents{{
  {-1.00f, 30.0f},
  {-0.55f, 40.0f},
  {-0.20f, 52.0f},
  {-0.12f, 59.0f},
  {-0.05f, 63.5f},
  {0.00f, 65.0f},
  {0.25f, 72.0f},
  {0.60f, 84.0f},
  {1.00f, 96.0f},
}};

constexpr std::array<vec2, 7> _mountains{{
  {-1.00f, 120.0f},
  {-0.75f, 95.0f},
  {-0.40f, 45.0f},
  {-0.20f, 18.0f},
  {0.00f, 6.0f},
  {0.40f, 2.0f},
  {1.00f, 0.0f},
}};

constexpr std::array<vec2, 4> _hills{{
  {-1.00f, 10.0f},
  {0.00f, 6.0f},
  {0.50f, 3.0f},
  {1.00f, 1.0f},
}};

constexpr f32 _baseRoughness     = 3.0f;
constexpr f32 _mountainRoughness = 0.25f;

f32 spline(std::span<const vec2> points, f32 x) {
  if (x <= points.front().x) return points.front().y;

  for (size i = 1; i < points.size(); ++i) {
    if (x > points[i].x) continue;
    const vec2 a = points[i - 1];
    const vec2 b = points[i];
    return glm::mix(a.y, b.y, (x - a.x) / (b.x - a.x));
  }

  return points.back().y;
}

f32 smoothstep(f32 from, f32 to, f32 x) {
  const f32 t = std::clamp((x - from) / (to - from), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

}

TerrainShape terrainShape(const ClimatePoint& climate) {
  TerrainShape shape;

  const f32 peaks = std::max(climate.peaksValleys, 0.0f);

  shape.inland   = smoothstep(-0.05f, 0.15f, climate.continentalness);
  shape.mountain = shape.inland * spline(_mountains, climate.erosion) * peaks;
  shape.height   = spline(_continents, climate.continentalness) + shape.mountain + shape.inland * spline(_hills, climate.erosion) * climate.peaksValleys;

  shape.river  = smoothstep(-0.75f, -0.95f, climate.peaksValleys) * smoothstep(-0.05f, 0.05f, climate.continentalness) * smoothstep(-0.5f, -0.3f, climate.erosion);
  shape.height = glm::mix(shape.height, static_cast<f32>(seaLevel - 4), shape.river);

  shape.swamp  = smoothstep(0.45f, 0.6f, climate.erosion) * shape.inland;
  shape.height = glm::mix(shape.height, static_cast<f32>(seaLevel + 1), shape.swamp * 0.85f);

  shape.roughness = _baseRoughness + shape.mountain * _mountainRoughness;
  return shape;
}
