#pragma once

#include "core/platform.h"

#include <vector>
#include <vulkan/vulkan.h>

struct VulkanInstance;
struct VulkanSurface;

struct VulkanDevice {
  VkPhysicalDevice physical_device;
  VkDevice logical_device;

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;

  u32 graphics_family_index;
  u32 present_family_index;
  u32 compute_family_index;
  u32 transfer_family_index;

  b8 create(VulkanInstance *instance, VulkanSurface *surface);
  void destroy();
  void waitIdle();
};
