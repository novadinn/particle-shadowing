#include "vulkan_descriptor_allocator.h"

#include "core/logger.h"
#include "vk_check.h"

VkDescriptorPool VulkanDescriptorAllocator::current_pool;
std::vector<VkDescriptorPool> VulkanDescriptorAllocator::used_pools;
std::vector<VkDescriptorPool> VulkanDescriptorAllocator::free_pools;

b8 VulkanDescriptorAllocator::initialize() {
  current_pool = VK_NULL_HANDLE;

  return true;
}

void VulkanDescriptorAllocator::shutdown(VulkanDevice *device) {
  for (u32 i = 0; i < free_pools.size(); ++i) {
    vkDestroyDescriptorPool(device->logical_device, free_pools[i], 0);
  }
  for (u32 i = 0; i < used_pools.size(); ++i) {
    vkDestroyDescriptorPool(device->logical_device, used_pools[i], 0);
  }
}

void VulkanDescriptorAllocator::reset(VulkanDevice *device) {
  for (u32 i = 0; i < used_pools.size(); ++i) {
    vkResetDescriptorPool(device->logical_device, used_pools[i], 0);
  }

  used_pools.clear();
  current_pool = VK_NULL_HANDLE;
}

b8 VulkanDescriptorAllocator::allocateDescriptorSet(
    VulkanDevice *device, VkDescriptorSetLayout layout,
    VkDescriptorSet *out_descriptor_set) {
  if (current_pool == VK_NULL_HANDLE) {
    current_pool = grabPool(device);
    used_pools.emplace_back(current_pool);
  }

  VkDescriptorSetAllocateInfo set_allocate_info = {};
  set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  set_allocate_info.pNext = 0;
  set_allocate_info.descriptorPool = current_pool;
  set_allocate_info.descriptorSetCount = 1;
  set_allocate_info.pSetLayouts = &layout;

  VkResult result = vkAllocateDescriptorSets(
      device->logical_device, &set_allocate_info, out_descriptor_set);
  switch (result) {
  case VK_SUCCESS: {
    return true;
  } break;
  case VK_ERROR_FRAGMENTED_POOL:
  case VK_ERROR_OUT_OF_POOL_MEMORY: {
    /* reallocate */
    current_pool = grabPool(device);
    used_pools.emplace_back(current_pool);
    VK_CHECK(vkAllocateDescriptorSets(device->logical_device,
                                      &set_allocate_info, out_descriptor_set));
    return true;
  } break;
  default: {
    FATAL("Unrecoverable error encountered while allocating descriptor set!");
    return false;
  } break;
  }

  return false;
}

VkDescriptorPool VulkanDescriptorAllocator::grabPool(VulkanDevice *device) {
  if (free_pools.size() > 0) {
    VkDescriptorPool pool = free_pools.back();
    free_pools.pop_back();
    return pool;
  }

  const u32 size_count = 1000;
  const std::vector<std::pair<VkDescriptorType, f32>> pool_sizes = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f}};

  std::vector<VkDescriptorPoolSize> sizes;
  sizes.reserve(pool_sizes.size());
  for (auto size : pool_sizes) {
    sizes.push_back({size.first, u32(size.second * size_count)});
  }
  VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
  descriptor_pool_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_create_info.flags = 0;
  descriptor_pool_create_info.maxSets = size_count;
  descriptor_pool_create_info.poolSizeCount = sizes.size();
  descriptor_pool_create_info.pPoolSizes = sizes.data();

  VkDescriptorPool pool;
  VK_CHECK(vkCreateDescriptorPool(device->logical_device,
                                  &descriptor_pool_create_info, 0, &pool));

  return pool;
}