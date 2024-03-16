#pragma once

#include "core/platform.h"

#include <vulkan/vulkan.h>

struct VulkanInstance;

struct VulkanDebugMessenger {
  VkDebugUtilsMessengerEXT handle;

  b8 create(VulkanInstance *instance);
  void destroy(VulkanInstance *instance);
};