#pragma once

#include "core/types.hpp"
#include "net/protocol.hpp"

enum class Button : u32 {
  Jump   = 1u << 0,
  Crouch = 1u << 1
};

struct Controls {
  vec2 move    = vec2(0.0f);
  f32  pitch   = 0.0f;
  f32  yaw     = 0.0f;
  u32  buttons = 0;

  bool held(Button button) const { return (buttons & static_cast<u32>(button)) != 0; }
  void press(Button button) { buttons |= static_cast<u32>(button); }

  static Controls       fromInput(const protocol::PlayerInput& input);
  protocol::PlayerInput toInput(u32 sequence) const;
};
