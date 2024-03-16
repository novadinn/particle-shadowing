#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

struct DescriptorLayoutInfo {
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  b8 operator==(const DescriptorLayoutInfo &other) const;
  size_t hash() const;
};

struct DescriptorLayoutHash {
  std::size_t operator()(const DescriptorLayoutInfo &info) const {
    return info.hash();
  }
};

struct VulkanDescriptorSetLayoutCache {
  static std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout,
                            DescriptorLayoutHash>
      layout_cache;

  static b8 initialize();
  static void shutdown(VulkanDevice *device);

  static VkDescriptorSetLayout
  layoutCreate(VulkanDevice *device,
               VkDescriptorSetLayoutCreateInfo *layout_create_info);
};

VkDescriptorSetLayoutCreateInfo
vulkanDescriptorSetLayoutCreateInfo(u32 binding_count,
                                    VkDescriptorSetLayoutBinding *bindings);