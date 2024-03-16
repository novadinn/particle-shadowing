#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vk_mem_alloc.h>

struct VulkanInstance;

struct VulkanMemoryAllocator {
  VmaAllocator handle;

  b8 create(VulkanInstance *instance, VulkanDevice *device, u32 api_version);
  void destroy();
};