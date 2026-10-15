#pragma once

#include "client/settings.hpp"
#include "game/player/controls.hpp"

namespace grf {
class Window;
}

class Controller {
  f32 m_pitch = 0.0f;
  f32 m_yaw   = 0.0f;

public:
  Controller()  = default;
  ~Controller() = default;

  Controller(const Controller&)            = delete;
  Controller(Controller&&)                 = delete;
  Controller& operator=(const Controller&) = delete;
  Controller& operator=(Controller&&)      = delete;

  Controls sample(const grf::Window& window, const Settings::Controls& settings, bool active);
  void     reset();
};
