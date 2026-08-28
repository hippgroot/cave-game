#include "game_objects.hpp"

Camera::Camera(vec3 position, vec3 target)
: m_position(position)
{
  vec3 dir = glm::normalize(target - m_position);
  m_pitch = std::asin(dir.y);
  m_yaw = std::atan2(dir.z, dir.x);
}

vec3 Camera::position() const {
  return m_position;
}

vec3 Camera::forward() const {
  f32 cpitch = std::cos(m_pitch);
  f32 spitch = std::sin(m_pitch);
  f32 cyaw = std::cos(m_yaw);
  f32 syaw = std::sin(m_yaw);

  return glm::normalize(vec3(
    cpitch * cyaw,
    spitch,
    cpitch * syaw
  ));
}

vec3 Camera::right() const {
  return glm::cross(forward(), m_worldUp);
}

vec3 Camera::up() const {
  return m_worldUp;
}

mat4 Camera::view() const {
  return glm::lookAt(m_position, m_position + forward(), m_worldUp);
}

mat4 Camera::proj(f32 fov, f32 aspect) const {
  mat4 m = glm::perspective(fov, aspect, m_near, m_far);
  m[1][1] *= -1;
  return m;
}

Camera::Frustrum Camera::frustrum(mat4 projView) const {
  auto row = [projView](i32 i){
    return vec4(projView[0][i], projView[1][i], projView[2][i], projView[3][i]);
  };

  return Frustrum{{
    row(3) + row(0),
    row(3) - row(0),
    row(3) + row(1),
    row(3) - row(1),
    row(2),
    row(3) - row(2)
  }};
}

void Camera::translate(vec3 dir) {
  m_position += dir.x * right() + dir.y * m_worldUp + dir.z * forward();
}

void Camera::rotate(f32 pitch, f32 yaw) {
  m_yaw += yaw;
  m_pitch += pitch;
  m_pitch = std::clamp(m_pitch, glm::radians(-89.0f), glm::radians(89.0f));
}

bool Camera::Frustrum::intersects(vec3 mn, vec3 mx) const {
  for (const vec4& p : planes) {
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