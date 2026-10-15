#include "game/world/chunk.hpp"

#include <algorithm>
#include <limits>

namespace {

constexpr size _runBytes = 3;
constexpr u32  _maxRun   = std::numeric_limits<u16>::max();

}

Chunk::Chunk()
: m_blocks(volume, Block::Air) {}

std::vector<u8> Chunk::encode() const {
  std::vector<u8> bytes;
  bytes.reserve(area);
  for (const Biome biome : m_biomes)
    bytes.push_back(static_cast<u8>(biome));

  auto flush = [&bytes](Block block, u32 run) {
    bytes.push_back(static_cast<u8>(run & 0xff));
    bytes.push_back(static_cast<u8>(run >> 8));
    bytes.push_back(static_cast<u8>(block));
  };

  Block current = m_blocks.front();
  u32   run     = 0;

  for (const Block block : m_blocks) {
    if (block != current || run == _maxRun) {
      flush(current, run);
      current = block;
      run     = 0;
    }
    ++run;
  }
  flush(current, run);

  return bytes;
}

std::optional<Chunk> Chunk::decode(std::span<const u8> bytes) {
  if (bytes.size() < static_cast<size>(area)) return std::nullopt;

  Chunk chunk;
  for (size i = 0; i < static_cast<size>(area); ++i) {
    if (bytes[i] >= static_cast<u8>(Biome::Count)) return std::nullopt;
    chunk.m_biomes[i] = static_cast<Biome>(bytes[i]);
  }

  bytes = bytes.subspan(area);
  if (bytes.size() % _runBytes != 0) return std::nullopt;

  size filled = 0;

  for (size i = 0; i < bytes.size(); i += _runBytes) {
    const u32 run   = static_cast<u32>(bytes[i]) | (static_cast<u32>(bytes[i + 1]) << 8);
    const u8  block = bytes[i + 2];

    if (run == 0 || block >= static_cast<u8>(Block::Count) || filled + run > static_cast<size>(volume)) return std::nullopt;

    std::fill_n(chunk.m_blocks.begin() + static_cast<std::ptrdiff_t>(filled), run, static_cast<Block>(block));
    filled += run;
  }

  if (filled != static_cast<size>(volume)) return std::nullopt;
  return chunk;
}
