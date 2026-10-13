#pragma once

#include "core/types.hpp"

#include <vector>

using MeshID = u32;

struct AABB {
  vec3 min;
  vec3 max;
};

struct MeshData {
  std::vector<byte> vertices;
  std::vector<u32>  indices;
  std::vector<byte> aux;
  std::vector<byte> pushConstants;
  AABB              aabb;
  str               pipeline;
  MeshID            meshID;
  u8                layer;
};
