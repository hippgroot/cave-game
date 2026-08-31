#pragma once

#include "./types.hpp"

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