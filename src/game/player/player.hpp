#pragma once

#include "game/player/body.hpp"
#include "game/player/controls.hpp"
#include "game/player/spectator.hpp"

#include <concepts>
#include <variant>

template <typename T>
concept PlayerState = std::copyable<T> && requires(const T state, PlayerBody& body, const Controls& controls, f32 dt) {
  { T::mode } -> std::convertible_to<PlayerMode>;
  state.enter(body);
  state.exit(body);
  state.step(body, controls, dt);
};

class Player {
public:
  using State = std::variant<SpectatorState>;

  static constexpr f32 eyeHeight   = 1.62f;
  static constexpr f32 maxPitch    = glm::radians(89.9f);
  static constexpr f32 worldBorder = 30'000'000.0f;
  static constexpr f32 minY        = -1024.0f;
  static constexpr f32 maxY        = 2048.0f;

  struct Info {
    vec3       position = vec3(0.0f);
    PlayerMode mode     = PlayerMode::Spectator;
  };

private:
  PlayerBody m_body;
  State      m_state;

public:
  explicit Player(const Info& info);
  ~Player() = default;

  Player(const Player&)            = default;
  Player(Player&&)                 = default;
  Player& operator=(const Player&) = default;
  Player& operator=(Player&&)      = default;

  void step(const Controls& controls, f32 dt);

  PlayerMode mode() const;
  void       setMode(PlayerMode mode);

  const PlayerBody& body() const;
  vec3              position() const;
  vec3              eye() const;
  void              place(vec3 position, vec3 velocity);

private:
  static State makeState(PlayerMode mode);
};
