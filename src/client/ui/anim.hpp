#pragma once

#include "core/types.hpp"

namespace ui {

using Easing = f32 (*)(f32);

namespace ease {

f32 linear(f32 t);
f32 inQuad(f32 t);
f32 outQuad(f32 t);
f32 inOutQuad(f32 t);
f32 inCubic(f32 t);
f32 outCubic(f32 t);
f32 inOutCubic(f32 t);
f32 outQuart(f32 t);
f32 outExpo(f32 t);
f32 outBack(f32 t);

}

f32 progress(f32 time, f32 delay, f32 duration);
f32 approach(f32 current, f32 target, f32 rate, f32 dt);

struct Tween {
  f32    delay    = 0.0f;
  f32    duration = 0.4f;
  Easing easing   = ease::outCubic;

  f32  linear(f32 time) const;
  f32  at(f32 time) const;
  bool done(f32 time) const;

  Tween staggered(u32 index, f32 step) const;
};

f32 smoothed(u32 id, f32 target, f32 rate, f32 dt);

}
