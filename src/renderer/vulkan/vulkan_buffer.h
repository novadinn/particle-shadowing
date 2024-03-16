#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct VulkanMemoryAllocator;
struct VulkanQueue;
struct VulkanCommandPool;

struct VulkanBuffer {
  VkBuffer handle;
  VmaAllocation memory;
  u32 size;

  b8 create(VulkanMemoryAllocator *allocator, u32 buffer_size,
            VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags,
            VmaMemoryUsage vma_usage);
  void destroy(VulkanMemoryAllocator *allocator);

  void *lock(VulkanMemoryAllocator *allocator);
  void unlock(VulkanMemoryAllocator *allocator);

  b8 loadData(VulkanMemoryAllocator *allocator, void *data);
  bool loadDataStaging(VulkanDevice *device, VulkanMemoryAllocator *allocator,
                       void *data, VulkanQueue *queue,
                       VulkanCommandPool *command_pool);
  bool copyTo(VulkanDevice *device, VulkanBuffer *dest, VulkanQueue *queue,
              VulkanCommandPool *command_pool);
};