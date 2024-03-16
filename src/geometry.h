#pragma once

#include "core/platform.h"

#include <vector>

std::vector<f32> generateSphereVertices(f32 radius, i32 sector_count,
                                        i32 stack_count) {
  std::vector<f32> vertices;

  const f32 PI = acos(-1.0f);

  f32 x, y, z, xy;                            // vertex position
  f32 nx, ny, nz, length_inv = 1.0f / radius; // normal
  f32 s, t;                                   // texCoord

  f32 sector_step = 2 * PI / sector_count;
  f32 stack_step = PI / stack_count;
  f32 sector_angle, stack_angle;

  for (u32 i = 0; i <= stack_count; ++i) {
    stack_angle = PI / 2 - i * stack_step; // starting from pi/2 to -pi/2
    xy = radius * cosf(stack_angle);       // r * cos(u)
    z = radius * sinf(stack_angle);        // r * sin(u)

    // add (sector_count+1) vertices per stack
    // the first and last vertices have same position and normal, but different
    // tex coords
    for (u32 j = 0; j <= sector_count; ++j) {
      sector_angle = j * sector_step; // starting from 0 to 2pi

      // vertex position
      x = xy * cosf(sector_angle); // r * cos(u) * cos(v)
      y = xy * sinf(sector_angle); // r * cos(u) * sin(v)
      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);
    }
  }

  return vertices;
}

std::vector<u32> generateSphereIndices(i32 sector_count, i32 stack_count) {
  std::vector<u32> indices;

  // indices
  //  k1--k1+1
  //  |  / |
  //  | /  |
  //  k2--k2+1
  u32 k1, k2;
  for (u32 i = 0; i < stack_count; ++i) {
    k1 = i * (sector_count + 1); // beginning of current stack
    k2 = k1 + sector_count + 1;  // beginning of next stack

    for (u32 j = 0; j < sector_count; ++j, ++k1, ++k2) {
      // 2 triangles per sector excluding 1st and last stacks
      if (i != 0) {
        indices.push_back(k1);
        indices.push_back(k2);
        indices.push_back(k1 + 1);
      }

      if (i != (stack_count - 1)) {
        indices.push_back(k1 + 1);
        indices.push_back(k2);
        indices.push_back(k2 + 1);
      }
    }
  }

  return indices;
}