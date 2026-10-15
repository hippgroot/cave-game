#include "game/player/spectator.hpp"

#include <cmath>

void SpectatorState::enter(PlayerBody&) const {}

void SpectatorState::exit(PlayerBody&) const {}

void SpectatorState::step(PlayerBody& body, const Controls& controls, f32 dt) const {
  vec3      horizontal = body.forward() * controls.move.y + body.right() * controls.move.x;
  const f32 length     = glm::length(horizontal);
  if (length > 1.0f) horizontal /= length;

  const f32  vertical = (controls.held(Button::Jump) ? 1.0f : 0.0f) - (controls.held(Button::Crouch) ? 1.0f : 0.0f);
  const vec3 wish     = (horizontal + vec3(0.0f, vertical, 0.0f)) * speed;

  body.velocity += (wish - body.velocity) * (1.0f - std::exp(-response * dt));
  body.position += body.velocity * dt;
}
