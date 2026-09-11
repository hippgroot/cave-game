#include "./world.hpp"

#include "FastNoiseLite.h"

#include <algorithm>

Chunk::Chunk(ivec2 coord, std::vector<Block> blocks)
: m_coord(coord), m_blocks(std::move(blocks))
{}

i32 Chunk::size() {
  return m_size;
}

i32 Chunk::height() {
  return m_height;
}

ivec2 Chunk::coord() const {
  return m_coord;
}

const std::vector<Block>& Chunk::blocks() const {
  return m_blocks;
}

World::World(Channel<Chunk>& chunkGenerated, JobPool& jobPool, Config config)
: m_chunkGenerated(chunkGenerated),
  m_jobPool(jobPool),
  m_config(std::move(config))
{}

const std::optional<Chunk>& World::operator[](ivec2 coord) const {
  return m_chunkCache.at(coord);
}

void World::update(ivec2 chunkCenter, i32 viewDistance) {
  static i32 _unloadOffset = 2;

  if (m_prevCenter.has_value() && *m_prevCenter == chunkCenter) {
    receiveChunks();
    return;
  }

  unloadChunks(chunkCenter, viewDistance + _unloadOffset);
  receiveChunks();
  requestChunks(chunkCenter, viewDistance);

  m_prevCenter = chunkCenter;
}

std::vector<ivec2> World::addedChunks() {
  return std::exchange(m_addedChunks, {});
}

std::vector<ivec2> World::removedChunks() {
  return std::exchange(m_removedChunks, {});
}

void World::unloadChunks(ivec2 center, i32 unloadDistance) {
  std::erase_if(m_chunkCache, [this, &center, &unloadDistance](const auto& elem){
    const auto& [coord, chunk] = elem;

    ivec2 d = center - coord;
    bool unload = std::max(std::abs(d.x), std::abs(d.y)) > unloadDistance;

    if (unload && chunk.has_value())
      m_removedChunks.emplace_back(coord);

    return unload;
  });
}

void World::receiveChunks() {
  while (auto chunkopt = m_chunkGenerated.receive()) {
    Chunk chunk = std::move(*chunkopt);
    if (!m_chunkCache.contains(chunk.coord())) continue;

    m_addedChunks.emplace_back(chunk.coord());
    m_chunkCache[chunk.coord()] = std::move(chunk);
  }
}

void World::requestChunks(ivec2 center, i32 viewDistance) {
  std::vector<ivec2> coords;
  for (i32 z = -viewDistance; z <= viewDistance; ++z) {
    for (i32 x = -viewDistance; x <= viewDistance; ++x) {
      ivec2 coord = ivec2(x, z);
      if (!m_chunkCache.contains(center + coord))
        coords.emplace_back(coord);
    }
  }

  std::stable_sort(coords.begin(), coords.end(), [](ivec2 a, ivec2 b){
    vec2 va = vec2(a), vb = vec2(b);
    return glm::dot(va, va) < glm::dot(vb, vb);
  });

  Channel<Chunk>& chunkGenerated = m_chunkGenerated;
  Config& config = m_config;

  for (auto coord : coords) {
    ivec2 chunkCoord = center + coord;
    m_chunkCache.emplace(chunkCoord, std::nullopt);

    m_jobPool.submit([&chunkGenerated, chunkCoord, config]{
      job_generateChunk(chunkGenerated, chunkCoord, config);
    });
  }
}

void World::job_generateChunk(Channel<Chunk>& chunkGenerated, ivec2 coord, Config config) {
  static const i32 _dirtDepth = 4;
  static const i32 _genSize = Chunk::size() + 2;
  static const i32 _planeSize = _genSize * _genSize;

  FastNoiseLite surfaceNoise;
  surfaceNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  surfaceNoise.SetFrequency(config.surfaceFrequency);
  surfaceNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
  surfaceNoise.SetFractalGain(config.surfaceGain);
  surfaceNoise.SetFractalLacunarity(config.surfaceLacunarity);
  surfaceNoise.SetFractalOctaves(config.surfaceOctaves);

  FastNoiseLite caveNoise;
  caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  caveNoise.SetFrequency(config.caveFrequency);

  vec2 planePos = (vec2(coord) + vec2(0.5f)) * static_cast<f32>(Chunk::size());
  vec3 center = vec3(planePos.x, 0.0f, planePos.y);

  i32 surfaceSeed = static_cast<i32>((config.seed >> 32) & 0xFFFFFFFF);
  i32 caveSeed = static_cast<i32>(config.seed & 0xFFFFFFFF);

  std::vector<i32> heights(_planeSize);
  for (i32 i = 0; i < static_cast<i32>(heights.size()); ++i) {
    i32 x = i % _genSize;
    i32 z = i / _genSize;

    f32 wx = center.x + static_cast<f32>(x - 1);
    f32 wz = center.z + static_cast<f32>(z - 1);

    f32 h = surfaceNoise.GetNoise(wx, wz) * 0.5f + 0.5f;
    heights[i] = static_cast<i32>(h * static_cast<f32>(Chunk::height() - 1));
  }

  std::vector<Block> blocks(_planeSize * Chunk::height(), Block::Air);
  for (i32 i = 0; auto& block : blocks) {
    i32 x = i % _genSize;
    i32 y = i / _planeSize;
    i32 z = (i / _genSize) % _genSize;
    ++i;

    i32 surfaceHeight = heights[z * _genSize + x];
    if (y > surfaceHeight) {
      block = Block::Air;
      continue;
    }

    f32 wx = center.x + static_cast<f32>(x - 1);
    f32 wz = center.z + static_cast<f32>(z - 1);

    bool caveMask = y > 0 && caveNoise.GetNoise(wx, static_cast<f32>(y), wz) > config.caveThreshold;

    if (caveMask)
      block = Block::Air;
    else if (y == surfaceHeight)
      block = Block::Grass;
    else if (y == 0)
      block = Block::Bedrock;
    else if (y + _dirtDepth > surfaceHeight)
      block = Block::Dirt;
    else
      block = Block::Stone;
  }

  chunkGenerated.send(Chunk(coord, std::move(blocks)));
  logging::general("Generated ({}, {})", coord.x, coord.y);
}