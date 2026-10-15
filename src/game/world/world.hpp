#pragma once

#include "game/world/chunk.hpp"
#include "game/world/gen/pipeline.hpp"

#include <memory>
#include <span>

class World {
public:
  struct Info {
    u64 seed = 0;
  };

private:
  u64           m_seed;
  ChunkPipeline m_pipeline;

public:
  explicit World(const Info& info);
  ~World() = default;

  World(const World&)            = delete;
  World(World&&)                 = delete;
  World& operator=(const World&) = delete;
  World& operator=(World&&)      = delete;

  u64 seed() const;

  void update(std::span<const ChunkLoader> loaders);

  std::shared_ptr<const Chunk> chunk(ChunkPos pos) const;
};
