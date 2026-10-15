#pragma once

#include "game/world/biome.hpp"
#include "game/world/gen/climate.hpp"
#include "game/world/gen/shape.hpp"

Biome pickBiome(const ClimatePoint& climate, const TerrainShape& shape);
