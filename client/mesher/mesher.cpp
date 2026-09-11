#include "./mesher.hpp"
#include "engine/render.hpp"
#include "engine/render/renderer/renderer.hpp"

Mesher::Mesher(Channel<MeshResult>& meshGenerated, JobPool& jobPool)
: m_meshGenerated(meshGenerated), m_jobPool(jobPool)
{}

std::vector<Mesher::MeshResult> Mesher::receiveMeshes() {
  std::vector<MeshResult> meshes;
  while (auto meshopt = m_meshGenerated.receive())
    meshes.emplace_back(std::move(*meshopt));
  return meshes;
}

void Mesher::newMesh(Chunk chunk) {
  static Renderer::MeshID _nextID = 1;

  Renderer::MeshID meshID = _nextID++;
  auto& meshGenerated = m_meshGenerated;

  m_jobPool.submit([&meshGenerated, chunk = std::move(chunk), meshID]() mutable {
    job_generateMesh(meshGenerated, std::move(chunk), meshID);
  });
}

void Mesher::job_generateMesh(Channel<MeshResult>& meshGenerated, Chunk chunk, Renderer::MeshID meshID) {
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

  ivec2 coord = chunk.coord();

  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::vector<u32> types;

  const auto& blocks = chunk.blocks();
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

        vec3 pos = origin + localCorner;
        vec2 uv = vec2((faceTile + localUV.x) / _atlasTiles, localUV.y);
        vec3 norm = _normals[face];

        vertices.emplace_back(Vertex{
          .pos  = pos,
          .uv   = uv,
          .norm = norm
        });
      }

      for (u32 idx : { 0u, 1u, 2u, 2u, 3u, 0u })
        indices.emplace_back(baseIndex + idx);

      types.emplace_back(static_cast<u32>(block));
    }
  }

  std::span<const std::byte> vertexBytes = std::as_bytes(std::span{vertices});
  std::span<const std::byte> auxBytes = std::as_bytes(std::span{types});
  std::span<const std::byte> pcBytes = std::as_bytes(std::span<const i32>({ chunk.coord().x, chunk.coord().y, Chunk::size() }));

  vec3 aabbMin = (vec3(coord.x, 0.0f, coord.y) - vec3(0.5f, 0.0f, 0.5f)) * static_cast<f32>(Chunk::size());
  AABB aabb{
    .min = aabbMin,
    .max = aabbMin + vec3(Chunk::size(), Chunk::height(), Chunk::size())
  };

  meshGenerated.send(std::make_pair(chunk.coord(), MeshData{
    .vertices       = std::vector<std::byte>(vertexBytes.begin(), vertexBytes.end()),
    .indices        = std::move(indices),
    .aux            = std::vector<std::byte>(auxBytes.begin(), auxBytes.end()),
    .pushConstants  = std::vector<std::byte>(pcBytes.begin(), pcBytes.end()),
    .aabb           = std::move(aabb),
    .pipeline       = "chunk_draw",
    .meshID         = meshID,
    .layer          = 0
  }));
  logging::general("Generated mesh ({}, {})", coord.x, coord.y);
}