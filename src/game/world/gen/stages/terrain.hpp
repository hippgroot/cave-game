#pragma once

#include "game/world/gen/climate.hpp"
#include "game/world/gen/stage.hpp"

#include <FastNoiseLite.h>

#include <memory>

class TerrainStage : public Stage {
  std::shared_ptr<const Climate> m_climate;
  FastNoiseLite                  m_noise;

public:
  TerrainStage(u64 seed, std::shared_ptr<const Climate> climate);
  ~TerrainStage() override = default;

  TerrainStage(const TerrainStage&)            = delete;
  TerrainStage(TerrainStage&&)                 = delete;
  TerrainStage& operator=(const TerrainStage&) = delete;
  TerrainStage& operator=(TerrainStage&&)      = delete;

  std::string_view name() const override { return "terrain"; }
  void             generate(Region& region) const override;
};
