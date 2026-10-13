#pragma once

#include "core/types.hpp"

class Settings {
  f32 m_sensitivity = 0.001f;
  f32 m_fov         = glm::radians(75.0f);

public:
  ~Settings() = default;

  Settings(const Settings&)            = delete;
  Settings(Settings&&)                 = delete;
  Settings& operator=(const Settings&) = delete;
  Settings& operator=(Settings&&)      = delete;

  inline void setFOV(f32 fov) { m_fov = fov; }
  inline void setSensitivity(f32 sensitivity) { m_sensitivity = sensitivity; }

  inline f32 getFOV() const { return m_fov; }
  inline f32 getSensitivity() const { return m_sensitivity; }
};
