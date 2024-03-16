#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vulkan/vulkan.h>

struct VulkanShaderModule {
  VkShaderModule handle;

  /* TODO: just pass a spirv binary here, move the loader logic elsewhere */
  b8 create(VulkanDevice *device, const char *path);
  void destroy(VulkanDevice *device);
};