#pragma once

#include "client/client_world.hpp"
#include "core/tasks.hpp"
#include "engine/mesh.hpp"
#include "game/world/chunk.hpp"

#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

class Mesher {
public:
  struct Changes {
    std::vector<MeshData> added;
    std::vector<MeshID>   removed;
  };

private:
  struct Result {
    std::optional<MeshData> opaque;
    std::optional<MeshData> translucent;
  };

  struct Entry {
    std::vector<MeshID> meshes;
    Task<Result>        task;
  };

  std::unordered_map<ChunkPos, Entry, ChunkPosHash> m_entries;
  Changes                                           m_changes;
  MeshID                                            m_nextID = 1;

public:
  Mesher()  = default;
  ~Mesher() = default;

  Mesher(const Mesher&)            = delete;
  Mesher(Mesher&&)                 = delete;
  Mesher& operator=(const Mesher&) = delete;
  Mesher& operator=(Mesher&&)      = delete;

  void    update(std::span<const ChunkEvent> events, const ClientWorld& world, ChunkPos center);
  void    clear();
  Changes take();

  size meshedChunks() const;
  size pendingChunks() const;

private:
  void submit(ChunkPos pos, const ClientWorld& world, ChunkPos center);
  void collect();
  void retire(Entry& entry);
  void add(Entry& entry, std::optional<MeshData> mesh);
};
