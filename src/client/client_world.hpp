#pragma once

#include "game/world/chunk.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

struct ChunkEvent {
  enum class Kind : u8 {
    Loaded,
    Unloaded
  };

  Kind     kind = Kind::Loaded;
  ChunkPos pos  = ChunkPos(0);
};

class ClientWorld {
  std::unordered_map<ChunkPos, std::shared_ptr<const Chunk>, ChunkPosHash> m_chunks;
  std::vector<ChunkEvent>                                                  m_events;

public:
  ClientWorld()  = default;
  ~ClientWorld() = default;

  ClientWorld(const ClientWorld&)            = delete;
  ClientWorld(ClientWorld&&)                 = delete;
  ClientWorld& operator=(const ClientWorld&) = delete;
  ClientWorld& operator=(ClientWorld&&)      = delete;

  void insert(ChunkPos pos, Chunk chunk);
  void erase(ChunkPos pos);

  std::shared_ptr<const Chunk> chunk(ChunkPos pos) const;
  size                         chunkCount() const;

  std::vector<ChunkEvent> takeEvents();
};
