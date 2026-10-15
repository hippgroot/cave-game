#pragma once

#include "core/tasks.hpp"
#include "game/world/chunk.hpp"
#include "game/world/gen/stage.hpp"

#include <chrono>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

struct ChunkLoader {
  ChunkPos center = ChunkPos(0);
  i32      radius = 0;

  bool operator==(const ChunkLoader&) const = default;
};

class ChunkPipeline {
public:
  using Status = u8;

  struct Info {
    u64       seed = 0;
    StageList stages;
  };

private:
  struct Entry {
    std::shared_ptr<ProtoChunk> proto;
    Status                      status    = 0;
    Status                      target    = 0;
    i32                         distance  = 0;
    i32                         overshoot = 0;
    u16                         readers   = 0;
    bool                        writer    = false;
  };

  struct Result {
    std::chrono::nanoseconds elapsed{0};
  };

  struct Job {
    ChunkPos              pos;
    Status                stage  = 0;
    Access                access = Access::None;
    std::vector<ChunkPos> neighbors;
    Task<Result>          task;
  };

  struct Stats {
    u64                      runs = 0;
    std::chrono::nanoseconds time{0};
  };

  using Entries = std::unordered_map<ChunkPos, Entry, ChunkPosHash>;

  std::shared_ptr<const StageList> m_stages;
  u64                              m_seed;
  Status                           m_complete;
  std::vector<i32>                 m_dependency;
  std::vector<i32>                 m_reach;
  std::vector<Stats>               m_stats;
  std::vector<ChunkLoader>         m_loaders;
  Entries                          m_entries;
  std::vector<Job>                 m_jobs;

public:
  explicit ChunkPipeline(Info info);
  ~ChunkPipeline();

  ChunkPipeline(const ChunkPipeline&)            = delete;
  ChunkPipeline(ChunkPipeline&&)                 = delete;
  ChunkPipeline& operator=(const ChunkPipeline&) = delete;
  ChunkPipeline& operator=(ChunkPipeline&&)      = delete;

  void update(std::span<const ChunkLoader> loaders);

  std::shared_ptr<const Chunk> chunk(ChunkPos pos) const;

private:
  void retarget();
  void collect();
  void unload();
  void schedule();

  void   measure(ChunkPos pos, Entry& entry) const;
  Status targetAt(i32 overshoot) const;
  bool   dependenciesMet(ChunkPos pos, const Entry& entry) const;
  bool   leasable(ChunkPos pos, const Stage::Requirements& requirements) const;
  void   submit(ChunkPos pos, Entry& entry);
  void   release(const Job& job);
  void   report() const;
};
