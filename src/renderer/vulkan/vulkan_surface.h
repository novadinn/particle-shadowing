#pragma once

#include "core/platform.h"

#include <SDL.h>
#include <vulkan/vulkan.h>

struct VulkanInstance;

struct VulkanSurface {
  VkSurfaceKHR handle;

  b8 create(VulkanInstance *instance, SDL_Window *window);
  void destroy(VulkanInstance *instance);
};