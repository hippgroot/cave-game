#pragma once

#include "game/world/gen/stage.hpp"

class FinalizeStage : public Stage {
public:
  FinalizeStage()           = default;
  ~FinalizeStage() override = default;

  FinalizeStage(const FinalizeStage&)            = delete;
  FinalizeStage(FinalizeStage&&)                 = delete;
  FinalizeStage& operator=(const FinalizeStage&) = delete;
  FinalizeStage& operator=(FinalizeStage&&)      = delete;

  std::string_view name() const override { return "finalize"; }
  void             generate(Region& region) const override;
};
