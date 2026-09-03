#pragma once

#include "engine/core.hpp"

enum class Block : u8 {
  Air,
  Grass,
  Dirt,
  Stone,
  Bedrock,
  Default = 255
};