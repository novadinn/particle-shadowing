#pragma once

#include "core/platform.h"

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#define NUM_PARTICLES 1024

struct Particle {
  glm::vec3 pos;
  f32 _pad0;
  f32 radius;
  f32 opacity;
  glm::vec2 _pad1;
};

struct ParticleSystem {
  Particle particles[NUM_PARTICLES];
  glm::vec3 velocities[NUM_PARTICLES];

  void createExplosion(glm::vec3 position) {
    for (u32 i = 0; i < NUM_PARTICLES; ++i) {
      particles[i].pos = position;
      particles[i].radius = glm::linearRand(0.1f, 0.5f);
      particles[i].opacity = 1.0f;
      velocities[i] = glm::ballRand(1.0);
    }
  }

  void update(f32 delta_time) {
    for (u32 i = 0; i < NUM_PARTICLES; ++i) {
      particles[i].pos += velocities[i] * delta_time;
      particles[i].opacity -= 0.5f * delta_time;
      if (particles[i].opacity < 0.0f) {
        particles[i].opacity = 0.0f;
      }
    }
  }
};