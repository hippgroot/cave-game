#include "world.hpp"

#include <algorithm>
#include <span>

Chunk::Chunk(u32 meshID, ivec2 coord, std::vector<Block> blocks)
: m_meshID(meshID),
  m_coord(coord),
  m_blocks(std::move(blocks))
{}

i32 Chunk::size() {
  return m_size;
}

i32 Chunk::height() {
  return m_height;
}

const ivec2& Chunk::coord() const {
  return m_coord;
}

u32 Chunk::meshID() const {
  return m_meshID;
}

World::World(Random& rand, Channel<GenRes>& chunkGenerated, JobPool& jobPool)
: m_random(rand),
  m_chunkGenerated(chunkGenerated),
  m_jobPool(jobPool)
{}

std::optional<Chunk>& World::operator[](ivec2 coord) {
  return m_chunks.at(coord);
}

const std::optional<Chunk>& World::operator[](ivec2 coord) const {
  return m_chunks.at(coord);
}

ivec2 World::getChunkCoord(vec3 position) {
  vec2 planarPos(position.x, position.z);
  return ivec2(glm::floor((planarPos - vec2(0.5f)) / static_cast<f32>(Chunk::size())));
}

vec3 World::getWorldPosition(ivec2 coord) {
  vec2 planarPos = (vec2(coord) + vec2(0.5f)) * static_cast<f32>(Chunk::size());
  return vec3(planarPos.x, 0.0f, planarPos.y);
}

void World::updateChunks(ivec2 currChunk, i32 viewDistance) {
  bool update = currChunk != m_currChunk;

  if (update) {
    m_currChunk = currChunk;
    unloadChunks(viewDistance + 2);
  }

  receiveChunks();

  if (update)
    requestChunks(viewDistance);
}

std::vector<MeshData> World::addedChunks() {
  return std::exchange(m_addedChunks, {});
}

std::vector<u32> World::removedChunks() {
  return std::exchange(m_removedChunks, {});
}

void World::job_generateChunk(Random& rand, Channel<GenRes>& chunkGenerated, ivec2 coord) {
  static u32 _meshID = 0u;

  auto blocks = generateBlocks(rand, coord);
  auto bufferData = generateMesh(blocks);

  auto vertices = std::as_bytes(std::span(bufferData.vertices.begin(), bufferData.vertices.end()));
  auto aux = std::as_bytes(std::span(bufferData.types.begin(), bufferData.types.end()));

  vec3 aabbMin = (vec3(coord.x, 0.0f, coord.y) - vec3(0.5f, 0.0f, 0.5f)) * static_cast<f32>(Chunk::size());
  AABB aabb{
    .min = aabbMin,
    .max = aabbMin + vec3(Chunk::size(), Chunk::height(), Chunk::size())
  };

  auto pc = std::as_bytes(std::span<const i32>({ coord.x, coord.y, Chunk::size() }));

  u32 meshID = _meshID++;
  MeshData meshData{
    .vertices = std::vector<std::byte>(vertices.begin(), vertices.end()),
    .indices  = std::move(bufferData.indices),
    .aux      = std::vector<std::byte>(aux.begin(), aux.end()),
    .pushConstants = std::vector<std::byte>(pc.begin(), pc.end()),
    .aabb     = std::move(aabb),
    .pipeline = "chunk_draw",
    .meshID   = meshID,
    .layer    = 0
  };

  GenRes res = std::make_pair(Chunk(meshID, coord, std::move(blocks)), std::move(meshData));
  chunkGenerated.send(std::move(res));

  logging::general("Generated chunk ({}, {})", coord.x, coord.y);
}

std::vector<Block> World::generateBlocks(Random& rand, ivec2 coord) {
  static const f32 _caveThreshold = 0.4f;
  static const i32 _dirtDepth = 4;
  static const i32 _genSize = Chunk::size() + 2;
  static const i32 _planeSize = _genSize * _genSize;

  vec3 center = getWorldPosition(coord);

  std::vector<i32> heights(_planeSize);
  for (i32 i = 0; i < static_cast<i32>(heights.size()); ++i) {
    i32 x = i % _genSize;
    i32 z = i / _genSize;

    f32 wx = center.x + static_cast<f32>(x - 1);
    f32 wz = center.z + static_cast<f32>(z - 1);

    f32 h = rand.surfaceNoise(wx, wz) * 0.5f + 0.5f;
    heights[i] = static_cast<i32>(h * static_cast<f32>(Chunk::height() - 1));
  }

  std::vector<Block> blocks(_planeSize * Chunk::height(), Block::Invalid);
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

    bool caveMask = y > 0 && rand.caveNoise(wx, static_cast<f32>(y), wz) > _caveThreshold;

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

  return blocks;
}

Chunk::Mesh World::generateMesh(const std::vector<Block>& blocks) {
  static const i32 _genSize = Chunk::size() + 2;
  static const i32 _planeSize = _genSize * _genSize;

  static const std::array<vec3, 6> _normals = {
    vec3(1.0f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f), vec3(0.0f,-1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f,-1.0f)
  };

  static const std::array<std::array<vec3, 4>, 6> _corners = {{
    { vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), vec3(1.0f, 0.0f, 1.0f) },
    { vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f) },
    { vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f), vec3(1.0f, 1.0f, 0.0f) },
    { vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
    { vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 1.0f, 1.0f) },
    { vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f) }
  }};

  static const std::array<std::array<vec2, 4>, 6> _cornerUV = {{
    { vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f), vec2(0.0f, 1.0f) },
    { vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f) },
    { vec2(0.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f) },
    { vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f) },
    { vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f) },
    { vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f), vec2(0.0f, 1.0f) }
  }};

  static const f32 _atlasTiles = 3.0f;
  static const std::array<f32, 6> _tiles = { 1.0f, 1.0f, 0.0f, 2.0f, 1.0f, 1.0f };

  static const std::array<ivec3, 6> _neighbors = {
    ivec3( 1, 0, 0), ivec3(-1, 0, 0),
    ivec3( 0, 1, 0), ivec3( 0,-1, 0),
    ivec3( 0, 0, 1), ivec3( 0, 0,-1)
  };

  std::vector<Chunk::Vertex> vertices;
  std::vector<u32> indices;
  std::vector<u32> types;

  for (u32 i = 0; const auto& block : blocks) {
    if (block == Block::Air) { ++i; continue; }

    i32 x = i % _genSize;
    i32 y = i / _planeSize;
    i32 z = (i / _genSize) % _genSize;
    ++i;

    if (x == 0 || x > Chunk::size()) continue;
    if (z == 0 || z > Chunk::size()) continue;

    vec3 origin = vec3(f32(x - 1), f32(y), f32(z - 1));

    for (u32 face = 0u; face < 6u; ++face) {
      if (y == 0 && face == 3u) continue;

      ivec3 neighbor = ivec3(x, y, z) + _neighbors[face];
      bool checkNeighbor =
        neighbor.x >= 0 && neighbor.x < _genSize        &&
        neighbor.y >= 0 && neighbor.y < Chunk::height() &&
        neighbor.z >= 0 && neighbor.z < _genSize;

      if (checkNeighbor) {
        uvec3 u = uvec3(neighbor);
        u32 j = u.y * _planeSize + u.z * _genSize + u.x;
        if (blocks[j] != Block::Air) continue;
      }

      u32 baseIndex = vertices.size();

      for (u32 corner = 0u; corner < 4u; ++corner) {
        vec2 localUV = _cornerUV[face][corner];
        vec3 localCorner = _corners[face][corner];
        f32 faceTile = _tiles[face];

        vertices.emplace_back(Chunk::Vertex{
          .pos  = origin + localCorner,
          .uv   = vec2((faceTile + localUV.x) / _atlasTiles, localUV.y),
          .norm = _normals[face]
        });
      }

      for (u32 idx : { 0u, 1u, 2u, 2u, 3u, 0u })
        indices.emplace_back(baseIndex + idx);

      types.emplace_back(static_cast<u32>(block));
    }
  }

  return Chunk::Mesh{
    .vertices = std::move(vertices),
    .indices  = std::move(indices),
    .types    = std::move(types)
  };
}

void World::unloadChunks(i32 unloadDistance) {
  std::erase_if(m_chunks, [this, &unloadDistance](const auto& elem){
    ivec2 d = m_currChunk - elem.first;
    bool unload = std::max(std::abs(d.x), std::abs(d.y)) > unloadDistance;

    if (unload && elem.second.has_value())
      m_removedChunks.emplace_back(elem.second->meshID());

    return unload;
  });
}

void World::receiveChunks() {
  while (auto genopt = m_chunkGenerated.receive()) {
    auto [chunk, meshData] = std::move(*genopt);
    if (!m_chunks.contains(chunk.coord())) continue;

    m_addedChunks.emplace_back(std::move(meshData));
    m_chunks[chunk.coord()] = std::move(chunk);
  }
}

void World::requestChunks(i32 viewDistance) {
  std::vector<ivec2> coords;
  for (i32 z = -viewDistance; z <= viewDistance; ++z) {
    for (i32 x = -viewDistance; x <= viewDistance; ++x) {
      ivec2 coord = ivec2(x, z);
      if (!m_chunks.contains(m_currChunk + coord))
        coords.emplace_back(coord);
    }
  }

  std::stable_sort(coords.begin(), coords.end(), [](ivec2 a, ivec2 b){
    vec2 va = vec2(a), vb = vec2(b);
    return glm::dot(va, va) < glm::dot(vb, vb);
  });

  Random& rand = m_random;
  Channel<GenRes>& chunkGenerated = m_chunkGenerated;

  for (auto coord : coords) {
    ivec2 chunkCoord = m_currChunk + coord;
    m_chunks.emplace(chunkCoord, std::nullopt);

    m_jobPool.submit([&rand, &chunkGenerated, chunkCoord](){
      job_generateChunk(rand, chunkGenerated, chunkCoord);
    });
  }
}