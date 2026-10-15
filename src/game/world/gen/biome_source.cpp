#include "game/world/gen/biome_source.hpp"

#include <array>

namespace {

constexpr std::array<f32, 4> _temperatureLevels{-0.45f, -0.15f, 0.2f, 0.55f};
constexpr std::array<f32, 4> _humidityLevels{-0.35f, -0.1f, 0.1f, 0.3f};

constexpr f32 _deepOcean  = -0.55f;
constexpr f32 _ocean      = -0.12f;
constexpr f32 _coast      = 0.0f;
constexpr f32 _stonyShore = -0.375f;
constexpr f32 _peaks      = 50.0f;
constexpr f32 _hills      = 20.0f;

using enum Biome;

constexpr std::array<std::array<Biome, 5>, 5> _middle{{
  {SnowyPlains, SnowyPlains, SnowyPlains, Taiga, Taiga},
  {Plains, Plains, Forest, Taiga, Taiga},
  {Plains, Plains, Forest, Forest, Forest},
  {Savanna, Savanna, Plains, Forest, Forest},
  {Desert, Desert, Desert, Savanna, Savanna},
}};

size level(f32 value, const std::array<f32, 4>& thresholds) {
  size result = 0;
  while (result < thresholds.size() && value >= thresholds[result])
    ++result;
  return result;
}

}

Biome pickBiome(const ClimatePoint& climate, const TerrainShape& shape) {
  if (climate.continentalness < _deepOcean) return DeepOcean;
  if (climate.continentalness < _ocean) return Ocean;
  if (shape.river > 0.5f) return River;
  if (climate.continentalness < _coast) return climate.erosion < _stonyShore ? StonyShore : Beach;

  const size temperature = level(climate.temperature, _temperatureLevels);
  const size humidity    = level(climate.humidity, _humidityLevels);

  if (shape.mountain > _peaks) return temperature <= 1 ? SnowySlopes : StonyPeaks;
  if (shape.mountain > _hills) return WindsweptHills;
  if (shape.swamp > 0.5f && temperature >= 2 && temperature <= 3 && humidity >= 2) return Swamp;

  return _middle[temperature][humidity];
}
