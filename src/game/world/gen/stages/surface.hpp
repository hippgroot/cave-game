#pragma once

#include "game/world/gen/stage.hpp"

#include <FastNoiseLite.h>

class SurfaceStage : public Stage {
  u64           m_seed;
  FastNoiseLite m_depthNoise;

public:
  explicit SurfaceStage(u64 seed);
  ~SurfaceStage() override = default;

  SurfaceStage(const SurfaceStage&)            = delete;
  SurfaceStage(SurfaceStage&&)                 = delete;
  SurfaceStage& operator=(const SurfaceStage&) = delete;
  SurfaceStage& operator=(SurfaceStage&&)      = delete;

  std::string_view name() const override { return "surface"; }
  Requirements     requirements() const override { return {.neighbors = Access::Read, .radius = 1}; }
  void             generate(Region& region) const override;
};
