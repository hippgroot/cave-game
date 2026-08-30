#pragma once

#include "core.hpp"

enum class Block : u8 {
  Air,
  Grass,
  Dirt,
  Stone,
  Bedrock,
  Invalid = std::numeric_limits<u8>::max()
};