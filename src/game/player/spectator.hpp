#pragma once

#include "game/player/body.hpp"
#include "game/player/controls.hpp"

class SpectatorState {
public:
  static constexpr PlayerMode mode     = PlayerMode::Spectator;
  static constexpr f32        speed    = 11.0f;
  static constexpr f32        response = 10.0f;

  SpectatorState()  = default;
  ~SpectatorState() = default;

  SpectatorState(const SpectatorState&)            = default;
  SpectatorState(SpectatorState&&)                 = default;
  SpectatorState& operator=(const SpectatorState&) = default;
  SpectatorState& operator=(SpectatorState&&)      = default;

  void enter(PlayerBody& body) const;
  void exit(PlayerBody& body) const;
  void step(PlayerBody& body, const Controls& controls, f32 dt) const;
};
