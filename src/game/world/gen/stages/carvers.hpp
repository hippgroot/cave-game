#pragma once

#include "game/world/gen/stage.hpp"

class CarverStage : public Stage {
  u64 m_seed;

public:
  explicit CarverStage(u64 seed);
  ~CarverStage() override = default;

  CarverStage(const CarverStage&)            = delete;
  CarverStage(CarverStage&&)                 = delete;
  CarverStage& operator=(const CarverStage&) = delete;
  CarverStage& operator=(CarverStage&&)      = delete;

  std::string_view name() const override { return "carvers"; }
  void             generate(Region& region) const override;
};
