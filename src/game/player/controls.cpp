#include "game/player/controls.hpp"

Controls Controls::fromInput(const protocol::PlayerInput& input) {
  return Controls{
    .move    = input.move,
    .pitch   = input.pitch,
    .yaw     = input.yaw,
    .buttons = input.buttons
  };
}

protocol::PlayerInput Controls::toInput(u32 sequence) const {
  return protocol::PlayerInput{
    .sequence = sequence,
    .move     = move,
    .pitch    = pitch,
    .yaw      = yaw,
    .buttons  = buttons
  };
}
