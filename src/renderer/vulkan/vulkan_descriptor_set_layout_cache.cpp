#include "vulkan_descriptor_set_layout_cache.h"

#include "vk_check.h"

std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout,
                   DescriptorLayoutHash>
    VulkanDescriptorSetLayoutCache::layout_cache;

b8 VulkanDescriptorSetLayoutCache::initialize() { return true; }

void VulkanDescriptorSetLayoutCache::shutdown(VulkanDevice *device) {
  for (auto pair : layout_cache) {
    vkDestroyDescriptorSetLayout(device->logical_device, pair.second, 0);
  }
}

VkDescriptorSetLayout VulkanDescriptorSetLayoutCache::layoutCreate(
    VulkanDevice *device, VkDescriptorSetLayoutCreateInfo *layout_create_info) {
  DescriptorLayoutInfo layout_info;
  layout_info.bindings.reserve(layout_create_info->bindingCount);
  b8 is_sorted = true;
  i32 last_binding = -1;

  for (u32 i = 0; i < layout_create_info->bindingCount; i++) {
    layout_info.bindings.push_back(layout_create_info->pBindings[i]);

    if (layout_create_info->pBindings[i].binding > last_binding) {
      last_binding = layout_create_info->pBindings[i].binding;
    } else {
      is_sorted = false;
    }
  }

  if (!is_sorted) {
    std::sort(
        layout_info.bindings.begin(), layout_info.bindings.end(),
        [](VkDescriptorSetLayoutBinding &a, VkDescriptorSetLayoutBinding &b) {
          return a.binding < b.binding;
        });
  }

  auto it = layout_cache.find(layout_info);
  if (it != layout_cache.end()) {
    return (*it).second;
  }

  VkDescriptorSetLayout layout;
  VK_CHECK(vkCreateDescriptorSetLayout(device->logical_device,
                                       layout_create_info, 0, &layout));

  layout_cache[layout_info] = layout;
  return layout;
}

b8 DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo &other) const {
  if (other.bindings.size() != bindings.size()) {
    return false;
  }

  for (u32 i = 0; i < bindings.size(); i++) {
    if (other.bindings[i].binding != bindings[i].binding) {
      return false;
    }
    if (other.bindings[i].descriptorType != bindings[i].descriptorType) {
      return false;
    }
    if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) {
      return false;
    }
    if (other.bindings[i].stageFlags != bindings[i].stageFlags) {
      return false;
    }
  }

  return true;
}

size_t DescriptorLayoutInfo::hash() const {
  using std::hash;
  using std::size_t;

  size_t result = hash<size_t>()(bindings.size());

  for (const VkDescriptorSetLayoutBinding &b : bindings) {
    size_t binding_hash = b.binding | b.descriptorType << 8 |
                          b.descriptorCount << 16 | b.stageFlags << 24;

    result ^= hash<size_t>()(binding_hash);
  }

  return result;
}

VkDescriptorSetLayoutCreateInfo
vulkanDescriptorSetLayoutCreateInfo(u32 binding_count,
                                    VkDescriptorSetLayoutBinding *bindings) {
  VkDescriptorSetLayoutCreateInfo layout_create_info = {};
  layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_create_info.pNext = 0;
  layout_create_info.flags = 0;
  layout_create_info.bindingCount = binding_count;
  layout_create_info.pBindings = bindings;

  return layout_create_info;
}