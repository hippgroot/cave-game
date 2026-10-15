#pragma once

#include "game/player/controls.hpp"
#include "game/player/player.hpp"
#include "net/protocol.hpp"

#include <deque>
#include <vector>

class PlayerPrediction {
  struct Pending {
    u32      sequence = 0;
    Controls controls;
  };

  Player              m_player;
  Player              m_previous;
  std::deque<Pending> m_pending;
  Controls            m_latest;
  u32                 m_acked;
  u32                 m_nextSequence;
  f32                 m_accumulator = 0.0f;
  vec3                m_correction  = vec3(0.0f);

public:
  explicit PlayerPrediction(const protocol::PlayerSync& sync);
  ~PlayerPrediction() = default;

  PlayerPrediction(const PlayerPrediction&)            = delete;
  PlayerPrediction(PlayerPrediction&&)                 = delete;
  PlayerPrediction& operator=(const PlayerPrediction&) = delete;
  PlayerPrediction& operator=(PlayerPrediction&&)      = delete;

  std::vector<protocol::PlayerInput> advance(f32 dt, const Controls& controls);
  void                               reconcile(const protocol::PlayerSync& sync);

  const Player& player() const;
  vec3          position() const;
  vec3          eye() const;
  f32           pitch() const;
  f32           yaw() const;
  size          pending() const;
  vec3          correction() const;

private:
  vec3 interpolated() const;
};
