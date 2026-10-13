#pragma once

#include "core/types.hpp"

#include <array>
#include <optional>
#include <utility>

class Camera {
public:
  class Frustum {
    std::array<vec4, 6> m_planes{};

  public:
    Frustum() = default;
    explicit Frustum(const mat4& projView);
    ~Frustum() = default;

    Frustum(const Frustum&)            = default;
    Frustum(Frustum&&)                 = default;
    Frustum& operator=(const Frustum&) = default;
    Frustum& operator=(Frustum&&)      = default;

    bool intersects(vec3 mn, vec3 mx) const;
  };

  struct Data {
    mat4  view;
    mat4  proj;
    mat4  projView;
    mat4  invView;
    mat4  invProj;
    vec3  position;
    vec3  forward;
    uvec2 viewport;
    f32   near;
    f32   far;
  };

  using UpdateData = std::pair<Frustum, Data>;

private:
  static constexpr f32  m_near         = 0.1f;
  static constexpr f32  m_far          = 1000.0f;
  static constexpr vec3 m_worldForward = vec3(0.0f, 0.0f, -1.0f);
  static constexpr vec3 m_worldRight   = vec3(1.0f, 0.0f, 0.0f);
  static constexpr vec3 m_worldUp      = vec3(0.0f, 1.0f, 0.0f);

  f32   m_fov      = glm::radians(70.0f);
  uvec2 m_viewport = uvec2(1u);

  vec3 m_position = vec3(0.0f);
  f32  m_pitch    = 0.0f;
  f32  m_yaw      = 0.0f;

  bool m_dirty = true;

public:
  Camera()  = default;
  ~Camera() = default;

  Camera(const Camera&)            = default;
  Camera(Camera&&)                 = default;
  Camera& operator=(const Camera&) = default;
  Camera& operator=(Camera&&)      = default;

  static f32  farPlane();
  static vec3 worldUp();
  static vec3 worldRight();

  vec3 position() const;

  std::optional<UpdateData> update();

  void follow(vec3 position, f32 pitch, f32 yaw);
  void setFOV(f32 fov_deg);
  void setViewport(u32 width, u32 height);
};
