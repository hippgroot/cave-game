#include "game/world/gen/stage.hpp"

#include <cassert>
#include <cstdlib>

namespace {

u64 mix(u64 value) {
  value += 0x9e3779b97f4a7c15ull;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
  return value ^ (value >> 31);
}

}

Region::Region(const Info& info)
: m_pos(info.pos),
  m_radius(info.radius),
  m_access(info.access),
  m_seed(info.seed),
  m_chunks(info.chunks) {
  assert(m_chunks.size() == static_cast<size>((2 * m_radius + 1) * (2 * m_radius + 1)));
}

ProtoChunk& Region::self() {
  return *m_chunks[slot(ivec2(0))];
}

const ProtoChunk& Region::read(ivec2 offset) const {
  assert(covers(offset));
  return *m_chunks[slot(offset)];
}

ProtoChunk& Region::write(ivec2 offset) {
  assert(covers(offset));
  assert(offset == ivec2(0) || m_access == Access::Write);
  return *m_chunks[slot(offset)];
}

bool Region::contains(ivec3 world) const {
  return world.y >= 0 && world.y < Chunk::height && covers(Chunk::posOf(world) - m_pos);
}

Block Region::get(ivec3 world) const {
  if (world.y < 0 || world.y >= Chunk::height) return Block::Air;

  const ivec2 offset = Chunk::posOf(world) - m_pos;
  assert(covers(offset));
  if (!covers(offset)) return Block::Air;

  return m_chunks[slot(offset)]->chunk.get(Chunk::localOf(world));
}

void Region::set(ivec3 world, Block block) {
  if (world.y < 0 || world.y >= Chunk::height) return;

  const ivec2 offset = Chunk::posOf(world) - m_pos;
  assert(covers(offset));
  assert(offset == ivec2(0) || m_access == Access::Write);
  if (!covers(offset) || (offset != ivec2(0) && m_access != Access::Write)) return;

  m_chunks[slot(offset)]->chunk.set(Chunk::localOf(world), block);
}

bool Region::covers(ivec2 offset) const {
  return std::abs(offset.x) <= m_radius && std::abs(offset.y) <= m_radius;
}

size Region::slot(ivec2 offset) const {
  const i32 side = 2 * m_radius + 1;
  return static_cast<size>((offset.y + m_radius) * side + (offset.x + m_radius));
}

u64 deriveSeed(u64 seed, std::string_view salt) {
  u64 hash = 0xcbf29ce484222325ull;
  for (const char c : salt) {
    hash ^= static_cast<u8>(c);
    hash *= 0x100000001b3ull;
  }
  return mix(seed ^ hash);
}

u64 deriveSeed(u64 seed, ChunkPos pos) {
  const u64 key = (static_cast<u64>(static_cast<u32>(pos.x)) << 32) | static_cast<u32>(pos.y);
  return mix(seed ^ mix(key));
}

i32 noiseSeed(u64 seed) {
  return static_cast<i32>(static_cast<u32>(seed ^ (seed >> 32)));
}
