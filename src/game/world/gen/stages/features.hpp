#pragma once

#include "game/world/gen/stage.hpp"

class FeatureStage : public Stage {
  u64 m_seed;

public:
  explicit FeatureStage(u64 seed);
  ~FeatureStage() override = default;

  FeatureStage(const FeatureStage&)            = delete;
  FeatureStage(FeatureStage&&)                 = delete;
  FeatureStage& operator=(const FeatureStage&) = delete;
  FeatureStage& operator=(FeatureStage&&)      = delete;

  std::string_view name() const override { return "features"; }
  Requirements     requirements() const override { return {.neighbors = Access::Write, .radius = 1}; }
  void             generate(Region& region) const override;
};
