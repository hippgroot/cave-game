#include "client/ui/anim.hpp"

#include <grf/imgui.h>

#include <algorithm>
#include <cmath>

namespace ui {

namespace ease {

f32 linear(f32 t) {
  return t;
}

f32 inQuad(f32 t) {
  return t * t;
}

f32 outQuad(f32 t) {
  return 1.0f - (1.0f - t) * (1.0f - t);
}

f32 inOutQuad(f32 t) {
  return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
}

f32 inCubic(f32 t) {
  return t * t * t;
}

f32 outCubic(f32 t) {
  return 1.0f - std::pow(1.0f - t, 3.0f);
}

f32 inOutCubic(f32 t) {
  return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
}

f32 outQuart(f32 t) {
  return 1.0f - std::pow(1.0f - t, 4.0f);
}

f32 outExpo(f32 t) {
  return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

f32 outBack(f32 t) {
  constexpr f32 _overshoot = 1.70158f;
  constexpr f32 _scale     = _overshoot + 1.0f;
  const f32     u          = t - 1.0f;
  return 1.0f + _scale * u * u * u + _overshoot * u * u;
}

}

f32 progress(f32 time, f32 delay, f32 duration) {
  if (duration <= 0.0f) return time >= delay ? 1.0f : 0.0f;
  return std::clamp((time - delay) / duration, 0.0f, 1.0f);
}

f32 approach(f32 current, f32 target, f32 rate, f32 dt) {
  return current + (target - current) * (1.0f - std::exp(-rate * dt));
}

f32 Tween::linear(f32 time) const {
  return progress(time, delay, duration);
}

f32 Tween::at(f32 time) const {
  return easing(linear(time));
}

bool Tween::done(f32 time) const {
  return time >= delay + duration;
}

Tween Tween::staggered(u32 index, f32 step) const {
  Tween tween = *this;
  tween.delay += step * static_cast<f32>(index);
  return tween;
}

f32 smoothed(u32 id, f32 target, f32 rate, f32 dt) {
  f32* value = ImGui::GetStateStorage()->GetFloatRef(id, target);
  *value     = approach(*value, target, rate, dt);
  return *value;
}

}
