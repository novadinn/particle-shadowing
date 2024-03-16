#include "camera.h"

#include <algorithm>

void Camera::create(f32 start_fov, f32 start_aspect_ratio, f32 start_near,
                    f32 start_far) {
  fov = start_fov;
  aspect_ratio = start_aspect_ratio;
  near = start_near;
  far = start_far;
}

void Camera::pan(const glm::vec2 &delta) {
  glm::vec2 speed = getPanSpeed();
  focal_point += -getRight() * delta.x * speed.x * distance;
  focal_point += -getUp() * delta.y * speed.y * distance;
}

void Camera::rotate(const glm::vec2 &delta) {
  f32 yaw_sign = getUp().y < 0 ? 1.0f : -1.0f;
  yaw += yaw_sign * delta.x * getRotationSpeed();
  pitch += delta.y * getRotationSpeed();
}

void Camera::zoom(f32 delta) {
  distance -= delta * getZoomSpeed();
  if (distance < 1.0f) {
    focal_point += getForward();
    distance = 1.0f;
  }
}

glm::mat4 Camera::getProjectionMatrix() {
  aspect_ratio = viewport_width / viewport_height;
  return glm::perspective(glm::radians(fov), aspect_ratio, near, far);
}

glm::mat4 Camera::getViewMatrix() {
  position = focal_point - getForward() * distance;

  glm::quat orientation = getOrientation();
  glm::mat4 view =
      glm::translate(glm::mat4(1.0f), position) * glm::toMat4(orientation);
  view = glm::inverse(view);

  return view;
}

glm::vec3 Camera::getUp() {
  return glm::rotate(getOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::getRight() {
  return glm::rotate(getOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Camera::getForward() {
  return glm::rotate(getOrientation(), glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::quat Camera::getOrientation() {
  return glm::quat(glm::vec3(-pitch, -yaw, 0.0f));
}

glm::vec2 Camera::getPanSpeed() {
  f32 x = std::min(viewport_width / 1000.0f, 2.4f);
  f32 x_factor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

  f32 y = std::min(viewport_height / 1000.0f, 2.4f);
  f32 y_factor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

  return {x_factor, y_factor};
}

f32 Camera::getRotationSpeed() { return 0.8f; }

f32 Camera::getZoomSpeed() {
  f32 dst = distance * 0.8f;
  dst = std::max(dst, 0.0f);
  f32 speed = dst * dst;
  speed = std::min(speed, 100.0f);

  return speed;
}
