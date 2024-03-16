#pragma once

#include "core/platform.h"

#include <SDL.h>
#include <vulkan/vulkan.h>

struct VulkanInstance {
  VkInstance handle;

  b8 create(VkApplicationInfo application_info, SDL_Window *window);
  void destroy();
};