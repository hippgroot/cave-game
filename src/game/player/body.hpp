#pragma once

#include "core/types.hpp"

#include <cmath>
#include <string_view>

enum class PlayerMode : u8 {
  Spectator,
  Count
};

std::string_view toString(PlayerMode mode);

struct PlayerBody {
  vec3 position = vec3(0.0f);
  vec3 velocity = vec3(0.0f);
  f32  pitch    = 0.0f;
  f32  yaw      = 0.0f;

  vec3 forward() const { return {-std::sin(yaw), 0.0f, -std::cos(yaw)}; }
  vec3 right() const { return {std::cos(yaw), 0.0f, -std::sin(yaw)}; }
};
