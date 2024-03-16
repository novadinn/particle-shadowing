#pragma once

#include "core/platform.h"
#include "vulkan_buffer.h"
#include "vulkan_device.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct VulkanCommandBuffer;
struct VulkanMemoryAllocator;
struct VulkanQueue;
struct VulkanCommandPool;

struct VulkanTexture {
  VkImage handle;
  VkImageView view;
  VmaAllocation memory;
  VkSampler sampler;
  VkFormat format;
  u32 width, height;

  b8 create(VulkanDevice *device, VulkanMemoryAllocator *allocator,
            VkFormat texture_format, u32 texture_width, u32 texture_height,
            VkImageUsageFlags usage_flags);
  void destroy(VulkanDevice *device, VulkanMemoryAllocator *allocator);

  b8 writeData(VulkanDevice *device, void *pixels,
               VulkanMemoryAllocator *allocator, VulkanQueue *queue,
               VulkanCommandPool *command_pool, u32 queue_family_index);
  b8 transitionLayout(VulkanCommandBuffer *command_buffer,
                      VkImageLayout old_layout, VkImageLayout new_layout,
                      u32 queue_family_index);
  b8 copyFromBuffer(VulkanBuffer *buffer, VulkanCommandBuffer *command_buffer);
};