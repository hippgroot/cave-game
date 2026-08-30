#pragma once

#include "block.hpp"

class Chunk {
public:
  struct Vertex {
    vec3 pos;
    vec2 uv;
    vec3 norm;
  };

  struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<u32>    indices;
    std::vector<u32>    types;
  };

private:
  static const i32 m_size = 16;
  static const i32 m_height = 256;

  ivec2               m_coord;
  std::vector<Block>  m_blocks;

public:
  Chunk(ivec2 coord, std::vector<Block> blocks);
  ~Chunk() = default;

  Chunk(const Chunk&) = default;
  Chunk(Chunk&&) = default;
  Chunk& operator=(const Chunk&) = default;
  Chunk& operator=(Chunk&&) = default;

  static i32 size();
  static i32 height();

  const ivec2& coord() const;
};