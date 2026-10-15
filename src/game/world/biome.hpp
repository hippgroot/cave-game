#pragma once

#include "core/types.hpp"
#include "game/world/block.hpp"

#include <string_view>

enum class Biome : u8 {
  Ocean,
  DeepOcean,
  River,
  Beach,
  StonyShore,
  Plains,
  Forest,
  Taiga,
  SnowyPlains,
  Desert,
  Savanna,
  Swamp,
  WindsweptHills,
  StonyPeaks,
  SnowySlopes,
  Count
};

struct BiomeInfo {
  std::string_view name;
  Block            top        = Block::Grass;
  Block            filler     = Block::Dirt;
  Block            underwater = Block::Dirt;
  Block            base       = Block::Stone;
  f32              trees      = 0.0f;
  bool             snowy      = false;
};

const BiomeInfo& biomeInfo(Biome biome);
