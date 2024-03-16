#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vulkan/vulkan.h>

struct VulkanCommandPool {
  VkCommandPool handle;

  b8 create(VulkanDevice *device, u32 family_index);
  void destroy(VulkanDevice *device);
};