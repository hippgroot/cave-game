#pragma once

#include "core/types.hpp"
#include "game/world/gen/climate.hpp"

inline constexpr i32 seaLevel          = 63;
inline constexpr f32 horizontalStretch = 2.0f;
inline constexpr f32 verticalScale     = 0.75f;

constexpr f32 squash(f32 height) {
  return static_cast<f32>(seaLevel) + (height - static_cast<f32>(seaLevel)) * verticalScale;
}

inline constexpr i32 snowLine = static_cast<i32>(squash(150.0f));

struct TerrainShape {
  f32 height    = 0.0f;
  f32 roughness = 0.0f;
  f32 inland    = 0.0f;
  f32 mountain  = 0.0f;
  f32 river     = 0.0f;
  f32 swamp     = 0.0f;
};

TerrainShape terrainShape(const ClimatePoint& climate);
