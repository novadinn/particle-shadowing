#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vector>
#include <vulkan/vulkan.h>

struct VulkanDescriptorAllocator {
  static VkDescriptorPool current_pool;
  static std::vector<VkDescriptorPool> used_pools;
  static std::vector<VkDescriptorPool> free_pools;

  static b8 initialize();
  static void shutdown(VulkanDevice *device);

  static void reset(VulkanDevice *device);

  static b8 allocateDescriptorSet(VulkanDevice *device,
                                  VkDescriptorSetLayout layout,
                                  VkDescriptorSet *out_descriptor_set);

  static VkDescriptorPool grabPool(VulkanDevice *device);
};