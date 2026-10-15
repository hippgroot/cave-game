#pragma once

#include "game/world/gen/climate.hpp"
#include "game/world/gen/stage.hpp"

#include <memory>

class BiomeStage : public Stage {
  std::shared_ptr<const Climate> m_climate;

public:
  explicit BiomeStage(std::shared_ptr<const Climate> climate);
  ~BiomeStage() override = default;

  BiomeStage(const BiomeStage&)            = delete;
  BiomeStage(BiomeStage&&)                 = delete;
  BiomeStage& operator=(const BiomeStage&) = delete;
  BiomeStage& operator=(BiomeStage&&)      = delete;

  std::string_view name() const override { return "biomes"; }
  void             generate(Region& region) const override;
};
