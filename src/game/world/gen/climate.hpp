#pragma once

#include "core/types.hpp"

#include <FastNoiseLite.h>

struct ClimatePoint {
  f32 temperature     = 0.0f;
  f32 humidity        = 0.0f;
  f32 continentalness = 0.0f;
  f32 erosion         = 0.0f;
  f32 weirdness       = 0.0f;
  f32 peaksValleys    = 0.0f;
};

class Climate {
  FastNoiseLite m_temperature;
  FastNoiseLite m_humidity;
  FastNoiseLite m_continentalness;
  FastNoiseLite m_erosion;
  FastNoiseLite m_weirdness;

public:
  explicit Climate(u64 seed);
  ~Climate() = default;

  Climate(const Climate&)            = delete;
  Climate(Climate&&)                 = delete;
  Climate& operator=(const Climate&) = delete;
  Climate& operator=(Climate&&)      = delete;

  ClimatePoint sample(i32 x, i32 z) const;
};
