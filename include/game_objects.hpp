#pragma once

#include "core.hpp"

class Camera {
public:
  struct Frustrum {
    std::array<grf::vec4, 6> planes;
    bool intersects(vec3 mn, vec3 mx) const;
  };

private:
  static constexpr vec3 m_worldUp = vec3(0.0f, 1.0f, 0.0f);

  vec3 m_position = vec3(0.0f);
  f32  m_pitch = 0.0f;
  f32  m_yaw = 0.0f;
  f32  m_near = 0.01f;
  f32  m_far = 10000.0f;

public:
  Camera(vec3 position = vec3(0.0f, 256.0f, 0.0f), vec3 target = vec3(0.0f, -5.0f, 10.0f));
  ~Camera() = default;

  Camera(const Camera&) = default;
  Camera(Camera&&) = default;
  Camera& operator=(const Camera&) = default;
  Camera& operator=(Camera&&) = default;

  vec3 position() const;
  vec3 right() const;
  vec3 forward() const;
  vec3 up() const;
  mat4 view() const;
  mat4 proj(f32 fov, f32 aspect) const;
  Frustrum frustrum(mat4 projView) const;

  void translate(vec3 delta);
  void rotate(f32 pitch, f32 yaw);
};