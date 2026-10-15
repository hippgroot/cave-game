#include "client/mesher.hpp"

#include "client/chunk_mesh.hpp"

#include <array>
#include <memory>
#include <unordered_set>

namespace {

constexpr std::array<ivec2, 4> _sides{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

}

void Mesher::update(std::span<const ChunkEvent> events, const ClientWorld& world, ChunkPos center) {
  std::unordered_set<ChunkPos, ChunkPosHash> dirty;

  for (const ChunkEvent& event : events) {
    if (event.kind == ChunkEvent::Kind::Unloaded) {
      dirty.erase(event.pos);
      if (auto itr = m_entries.find(event.pos); itr != m_entries.end()) {
        retire(itr->second);
        m_entries.erase(itr);
      }
      continue;
    }

    dirty.insert(event.pos);
    for (const ivec2 side : _sides)
      dirty.insert(event.pos + side);
  }

  for (const ChunkPos pos : dirty)
    submit(pos, world, center);

  collect();
}

void Mesher::clear() {
  for (auto& [pos, entry] : m_entries)
    retire(entry);
  m_entries.clear();
}

Mesher::Changes Mesher::take() {
  return std::exchange(m_changes, {});
}

size Mesher::meshedChunks() const {
  size count = 0;
  for (const auto& [pos, entry] : m_entries)
    if (!entry.meshes.empty()) ++count;
  return count;
}

size Mesher::pendingChunks() const {
  size count = 0;
  for (const auto& [pos, entry] : m_entries)
    if (entry.task.valid()) ++count;
  return count;
}

void Mesher::submit(ChunkPos pos, const ClientWorld& world, ChunkPos center) {
  auto chunk = world.chunk(pos);
  if (chunk == nullptr) return;

  std::array<std::shared_ptr<const Chunk>, 4> neighbors;
  for (size i = 0; i < _sides.size(); ++i) {
    neighbors[i] = world.chunk(pos + _sides[i]);
    if (neighbors[i] == nullptr) return;
  }

  const ivec2 offset   = pos - center;
  const i32   priority = offset.x * offset.x + offset.y * offset.y;

  m_entries[pos].task = Task<Result>([pos, chunk = std::move(chunk), neighbors = std::move(neighbors)] {
    ChunkMeshes meshes = buildChunkMeshes(*chunk, {neighbors[0].get(), neighbors[1].get(), neighbors[2].get(), neighbors[3].get()});

    Result result;
    if (!meshes.opaque.empty()) result.opaque = toMeshData(pos, std::move(meshes.opaque), false);
    if (!meshes.translucent.empty()) result.translucent = toMeshData(pos, std::move(meshes.translucent), true);
    return result;
  },
    priority);
}

void Mesher::collect() {
  for (auto& [pos, entry] : m_entries) {
    if (!entry.task.ready()) continue;

    std::optional<Result> result = entry.task.take();
    if (!result) continue;

    retire(entry);
    add(entry, std::move(result->opaque));
    add(entry, std::move(result->translucent));
  }
}

void Mesher::retire(Entry& entry) {
  m_changes.removed.insert(m_changes.removed.end(), entry.meshes.begin(), entry.meshes.end());
  entry.meshes.clear();
}

void Mesher::add(Entry& entry, std::optional<MeshData> mesh) {
  if (!mesh) return;

  mesh->meshID = m_nextID++;
  entry.meshes.push_back(mesh->meshID);
  m_changes.added.push_back(std::move(*mesh));
}
