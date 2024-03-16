#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vulkan/vulkan.h>

struct VulkanSemaphore {
  VkSemaphore handle;

  b8 create(VulkanDevice *device);
  void destroy(VulkanDevice *device);
};