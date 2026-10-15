#include "client/client_world.hpp"

#include <utility>

void ClientWorld::insert(ChunkPos pos, Chunk chunk) {
  m_chunks.insert_or_assign(pos, std::make_shared<const Chunk>(std::move(chunk)));
  m_events.push_back(ChunkEvent{.kind = ChunkEvent::Kind::Loaded, .pos = pos});
}

void ClientWorld::erase(ChunkPos pos) {
  if (m_chunks.erase(pos) == 0) return;
  m_events.push_back(ChunkEvent{.kind = ChunkEvent::Kind::Unloaded, .pos = pos});
}

std::shared_ptr<const Chunk> ClientWorld::chunk(ChunkPos pos) const {
  const auto itr = m_chunks.find(pos);
  return itr == m_chunks.end() ? nullptr : itr->second;
}

size ClientWorld::chunkCount() const {
  return m_chunks.size();
}

std::vector<ChunkEvent> ClientWorld::takeEvents() {
  return std::exchange(m_events, {});
}
