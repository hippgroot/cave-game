#pragma once

#include "engine/render.hpp"
#include "game/world/chunk.hpp"

struct Vertex {
  vec3 pos;
  vec2 uv;
  vec3 norm;
};

class Mesher {
public:
  using MeshResult = std::pair<ivec2, MeshData>;

private:
  Channel<MeshResult>&  m_meshGenerated;
  JobPool&              m_jobPool;

public:
  Mesher(Channel<MeshResult>& meshGenerated, JobPool& jobPool);
  ~Mesher() = default;

  Mesher(const Mesher&) = delete;
  Mesher(Mesher&&) = delete;
  Mesher& operator=(const Mesher&) = delete;
  Mesher& operator=(Mesher&&) = delete;

  std::vector<MeshResult> receiveMeshes();
  void newMesh(Chunk chunk);

private:
  static void job_generateMesh(Channel<MeshResult>& meshGenerated, Chunk chunk, Renderer::MeshID meshID);
};