#pragma once

#include "core/types.hpp"
#include "game/world/gen/climate.hpp"

inline constexpr i32 seaLevel = 63;
inline constexpr i32 snowLine = 150;

struct TerrainShape {
  f32 height    = 0.0f;
  f32 roughness = 0.0f;
  f32 inland    = 0.0f;
  f32 mountain  = 0.0f;
  f32 river     = 0.0f;
  f32 swamp     = 0.0f;
};

TerrainShape terrainShape(const ClimatePoint& climate);
