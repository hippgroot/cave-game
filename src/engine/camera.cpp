#include "engine/camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

f32 Camera::farPlane() {
  return m_far;
}

vec3 Camera::worldUp() {
  return m_worldUp;
}

vec3 Camera::worldRight() {
  return m_worldRight;
}

vec3 Camera::position() const {
  return m_position;
}

std::optional<Camera::UpdateData> Camera::update() {
  if (!m_dirty) return std::nullopt;

  quat rotation = glm::angleAxis(m_yaw, m_worldUp) * glm::angleAxis(m_pitch, m_worldRight);

  vec3 forward = rotation * m_worldForward;
  mat4 view = glm::lookAt(m_position, m_position + forward, m_worldUp);

  f32 aspect = static_cast<f32>(m_viewport.x) / static_cast<f32>(m_viewport.y);
  mat4 proj = glm::perspective(m_fov, aspect, m_near, m_far);
  proj[1][1] *= -1;

  mat4 projView = proj * view;

  mat4 invView = glm::translate(mat4(1), m_position) * glm::mat4_cast(rotation);

  f32 t = std::tan(m_fov * 0.5f);
  mat4 invProj(0.0f);
  invProj[0][0] = aspect * t;
  invProj[1][1] = -t;
  invProj[3][2] = -1.0f;
  invProj[2][3] = (m_near - m_far) / (m_far * m_near);
  invProj[3][3] = 1.0f / m_near;

  m_dirty = false;
  return std::make_pair(Frustum(projView), Data{
    .view     = view,
    .proj     = proj,
    .projView = projView,
    .invView  = invView,
    .invProj  = invProj,
    .position = m_position,
    .forward  = forward,
    .viewport = m_viewport,
    .near     = m_near,
    .far      = m_far
  });
}

void Camera::follow(vec3 position, f32 pitch, f32 yaw) {
  if (position == m_position && pitch == m_pitch && yaw == m_yaw) return;

  m_position = position;
  m_pitch = pitch;
  m_yaw = yaw;

  m_dirty = true;
}

void Camera::setFOV(f32 fov_deg) {
  m_fov = glm::radians(fov_deg);
  m_dirty = true;
}

void Camera::setViewport(u32 width, u32 height) {
  m_viewport = uvec2(width, height);
  m_dirty = true;
}

Camera::Frustum::Frustum(const mat4& projView) {
  auto row = [&projView](i32 i){
    return vec4(projView[0][i], projView[1][i], projView[2][i], projView[3][i]);
  };

  m_planes = {
    row(3) + row(0),
    row(3) - row(0),
    row(3) + row(1),
    row(3) - row(1),
    row(2),
    row(3) - row(2)
  };
}

bool Camera::Frustum::intersects(vec3 mn, vec3 mx) const {
  for (const vec4& p : m_planes) {
    vec3 v(
      p.x > 0.0f ? mx.x : mn.x,
      p.y > 0.0f ? mx.y : mn.y,
      p.z > 0.0f ? mx.z : mn.z
    );
    if (glm::dot(vec3(p), v) + p.w < 0.0f)
      return false;
  }
  return true;
}
