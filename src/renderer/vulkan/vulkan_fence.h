#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vulkan/vulkan.h>

struct VulkanFence {
  VkFence handle;

  b8 create(VulkanDevice *device);
  void destroy(VulkanDevice *device);

  void wait(VulkanDevice *device, u64 timeout);
  void reset(VulkanDevice *device);
};