#include "game/world/gen/stages/biomes.hpp"

#include "game/world/gen/biome_source.hpp"

BiomeStage::BiomeStage(std::shared_ptr<const Climate> climate)
: m_climate(std::move(climate)) {}

void BiomeStage::generate(Region& region) const {
  ProtoChunk& proto  = region.self();
  const ivec3 origin = region.origin();

  for (i32 z = 0; z < Chunk::width; ++z) {
    for (i32 x = 0; x < Chunk::width; ++x) {
      const ClimatePoint climate = m_climate->sample(origin.x + x, origin.z + z);
      proto.chunk.setBiome(x, z, pickBiome(climate, terrainShape(climate)));
    }
  }
}
