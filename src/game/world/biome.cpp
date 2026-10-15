#include "game/world/biome.hpp"

#include <array>

namespace {

constexpr std::array<BiomeInfo, static_cast<size>(Biome::Count)> _biomes{{
  {.name = "ocean", .top = Block::Sand, .filler = Block::Sand, .underwater = Block::Sand},
  {.name = "deep_ocean", .top = Block::Gravel, .filler = Block::Gravel, .underwater = Block::Gravel},
  {.name = "river", .underwater = Block::Sand},
  {.name = "beach", .top = Block::Sand, .filler = Block::Sand, .underwater = Block::Sand, .base = Block::Sandstone},
  {.name = "stony_shore", .top = Block::Stone, .filler = Block::Stone, .underwater = Block::Gravel},
  {.name = "plains", .trees = 0.05f},
  {.name = "forest", .trees = 8.0f},
  {.name = "taiga", .trees = 6.0f},
  {.name = "snowy_plains", .top = Block::Snow, .trees = 0.1f, .snowy = true},
  {.name = "desert", .top = Block::Sand, .filler = Block::Sand, .underwater = Block::Sand, .base = Block::Sandstone},
  {.name = "savanna", .trees = 0.8f},
  {.name = "swamp", .trees = 2.0f},
  {.name = "windswept_hills", .trees = 0.3f},
  {.name = "stony_peaks", .top = Block::Stone, .filler = Block::Stone, .underwater = Block::Gravel},
  {.name = "snowy_slopes", .top = Block::Snow, .filler = Block::Snow, .underwater = Block::Gravel, .snowy = true},
}};

}

const BiomeInfo& biomeInfo(Biome biome) {
  return _biomes[static_cast<size>(biome) < _biomes.size() ? static_cast<size>(biome) : 0];
}
