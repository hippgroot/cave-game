#pragma once

#include "types.hpp"
#include "grf/resources.hpp"

struct AABB {
  vec3 min;
  vec3 max;
};

struct MeshData {
  std::vector<std::byte>  vertices;
  std::vector<u32>        indices;
  std::vector<std::byte>  aux;
  std::vector<std::byte>  pushConstants;
  AABB                    aabb;
  str                     pipeline;
  u32                     meshID;
  u8                      layer;
};

struct Mesh {
  grf::Buffer                 vertexBuffer;
  grf::Buffer                 indexBuffer;
  std::optional<grf::Buffer>  auxBuffer;
  std::vector<std::byte>      pushConstants;
  AABB                        aabb;
  u32                         indexCount;
  u8                          layer;
  u8                          pipeline;
};