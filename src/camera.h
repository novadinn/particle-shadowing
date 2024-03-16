#pragma once

#include "core/platform.h"

#include "glm/gtx/quaternion.hpp"
#include <glm/glm.hpp>

struct Camera {
  f32 fov = 45.0f, near = 0.1f, far = 1000.0f;
  glm::vec3 position;
  f32 pitch = 0.0f, yaw = 0.0f;
  glm::vec3 focal_point = {0, 0, 0};
  f32 aspect_ratio = 1.778f;
  f32 distance = 10.0f;
  f32 viewport_width = 800, viewport_height = 600;

  void create(f32 start_fov, f32 start_aspect_ratio, f32 start_near,
              f32 start_far);

  void pan(const glm::vec2 &delta);
  void rotate(const glm::vec2 &delta);
  void zoom(f32 delta);

  glm::mat4 getProjectionMatrix();
  glm::mat4 getViewMatrix();

  glm::vec3 getUp();
  glm::vec3 getRight();
  glm::vec3 getForward();
  glm::quat getOrientation();

  glm::vec2 getPanSpeed();
  f32 getRotationSpeed();
  f32 getZoomSpeed();
};