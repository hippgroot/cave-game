#pragma once

#include "./block.hpp"

class Chunk {
  static constexpr i32 m_size = 16;
  static constexpr i32 m_height = 256;

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

  ivec2 coord() const;
};