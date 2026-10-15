#pragma once

#include "core/types.hpp"
#include "game/world/chunk.hpp"

#include <array>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

struct ProtoChunk {
  Chunk                        chunk;
  std::array<i16, Chunk::area> heightmap{};

  static constexpr i32 column(i32 x, i32 z) { return Chunk::column(x, z); }
};

enum class Access : u8 {
  None,
  Read,
  Write
};

class Region {
public:
  struct Info {
    ChunkPos                                     pos    = ChunkPos(0);
    i32                                          radius = 0;
    Access                                       access = Access::None;
    u64                                          seed   = 0;
    std::span<const std::shared_ptr<ProtoChunk>> chunks;
  };

private:
  ChunkPos                                     m_pos;
  i32                                          m_radius;
  Access                                       m_access;
  u64                                          m_seed;
  std::span<const std::shared_ptr<ProtoChunk>> m_chunks;

public:
  explicit Region(const Info& info);
  ~Region() = default;

  Region(const Region&)            = delete;
  Region(Region&&)                 = delete;
  Region& operator=(const Region&) = delete;
  Region& operator=(Region&&)      = delete;

  ChunkPos pos() const { return m_pos; }
  i32      radius() const { return m_radius; }
  u64      seed() const { return m_seed; }
  ivec3    origin() const { return Chunk::origin(m_pos); }

  ProtoChunk&       self();
  const ProtoChunk& read(ivec2 offset) const;
  ProtoChunk&       write(ivec2 offset);

  bool  contains(ivec3 world) const;
  Block get(ivec3 world) const;
  void  set(ivec3 world, Block block);

private:
  bool covers(ivec2 offset) const;
  size slot(ivec2 offset) const;
};

class Stage {
public:
  struct Requirements {
    Access neighbors = Access::None;
    i32    radius    = 0;
  };

  Stage()          = default;
  virtual ~Stage() = default;

  Stage(const Stage&)            = delete;
  Stage(Stage&&)                 = delete;
  Stage& operator=(const Stage&) = delete;
  Stage& operator=(Stage&&)      = delete;

  virtual std::string_view name() const = 0;
  virtual Requirements     requirements() const { return {}; }
  virtual void             generate(Region& region) const = 0;
};

using StageList = std::vector<std::unique_ptr<const Stage>>;

u64 deriveSeed(u64 seed, std::string_view salt);
u64 deriveSeed(u64 seed, ChunkPos pos);
i32 noiseSeed(u64 seed);
