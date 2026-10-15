#include "game/world/gen/stages.hpp"

#include "game/world/gen/climate.hpp"
#include "game/world/gen/stages/biomes.hpp"
#include "game/world/gen/stages/carvers.hpp"
#include "game/world/gen/stages/features.hpp"
#include "game/world/gen/stages/finalize.hpp"
#include "game/world/gen/stages/surface.hpp"
#include "game/world/gen/stages/terrain.hpp"

StageList makeStages(u64 seed) {
  const auto climate = std::make_shared<const Climate>(seed);

  StageList stages;
  stages.push_back(std::make_unique<BiomeStage>(climate));
  stages.push_back(std::make_unique<TerrainStage>(seed, climate));
  stages.push_back(std::make_unique<SurfaceStage>(seed));
  stages.push_back(std::make_unique<CarverStage>(seed));
  stages.push_back(std::make_unique<FeatureStage>(seed));
  stages.push_back(std::make_unique<FinalizeStage>());
  return stages;
}
