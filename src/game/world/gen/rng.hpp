#pragma once

#include "core/types.hpp"

class Rng {
  u64 m_state;

public:
  explicit Rng(u64 seed)
  : m_state(seed) {}
  ~Rng() = default;

  Rng(const Rng&)            = default;
  Rng(Rng&&)                 = default;
  Rng& operator=(const Rng&) = default;
  Rng& operator=(Rng&&)      = default;

  u64 next() {
    u64 value = (m_state += 0x9e3779b97f4a7c15ull);
    value     = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
    value     = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
    return value ^ (value >> 31);
  }

  i32 nextInt(i32 bound) {
    if (bound <= 0) return 0;
    return static_cast<i32>(((next() >> 32) * static_cast<u64>(bound)) >> 32);
  }

  f32  nextFloat() { return static_cast<f32>(next() >> 40) * 0x1.0p-24f; }
  bool chance(f32 probability) { return nextFloat() < probability; }
};
