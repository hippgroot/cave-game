#pragma once

#include "./types.hpp"

#include <mutex>
#include <random>

class Random {
  std::mutex          m_mutex;
  std::random_device  m_rd;
  std::mt19937_64     m_gen{m_rd()};

public:
  Random() = default;
  ~Random() = default;

  i32 i(i32 min = std::numeric_limits<i32>::min(), i32 max = std::numeric_limits<i32>::max());
  f32 f(f32 min = 0.0f, f32 max = 1.0f);
};