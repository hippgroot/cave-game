#pragma once

#include "engine/core.hpp"

#include "FastNoiseLite.h"

#include <mutex>
#include <random>

class Random {
  std::mutex          m_mutex;
  std::random_device  m_rd;
  std::mt19937_64     m_gen;
  FastNoiseLite       m_surfaceNoise;
  FastNoiseLite       m_caveNoise;
  u64                 m_seed;

public:
  Random() : m_gen(m_rd()), m_seed(static_cast<u64>(m_gen())) {
    u32 surfaceSeed = m_seed & 0xFFFFFFFFul;
    u32 caveSeed = (m_seed >> 32) & 0xFFFFFFFFul;

    m_surfaceNoise.SetSeed(surfaceSeed);
    m_surfaceNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_surfaceNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_surfaceNoise.SetFrequency(0.005f);
    m_surfaceNoise.SetFractalOctaves(5);
    m_surfaceNoise.SetFractalGain(0.5f);
    m_surfaceNoise.SetFractalLacunarity(2.0f);

    m_caveNoise.SetSeed(caveSeed);
    m_caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveNoise.SetFrequency(0.08f);
  };

  template <typename T = u32>
  T i(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
    std::lock_guard lock(m_mutex);
    return std::uniform_int_distribution<T>(min, max)(m_gen);
  }

  template <typename T = f32>
  T f(T min = 0.0, T max = 1.0) {
    std::lock_guard lock(m_mutex);
    return std::uniform_real_distribution<T>(min, max)(m_gen);
  }

  inline f32 surfaceNoise(f32 x, f32 z) const {
    return m_surfaceNoise.GetNoise(x, z);
  }

  inline f32 caveNoise(f32 x, f32 y, f32 z) const {
    return m_caveNoise.GetNoise(x, y, z);
  }
};