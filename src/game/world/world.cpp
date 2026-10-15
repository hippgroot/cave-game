#include "game/world/world.hpp"

#include "game/world/gen/stages.hpp"

World::World(const Info& info)
: m_seed(info.seed),
  m_pipeline(ChunkPipeline::Info{.seed = info.seed, .stages = makeStages(info.seed)}) {}

u64 World::seed() const {
  return m_seed;
}

void World::update(std::span<const ChunkLoader> loaders) {
  m_pipeline.update(loaders);
}

std::shared_ptr<const Chunk> World::chunk(ChunkPos pos) const {
  return m_pipeline.chunk(pos);
}
