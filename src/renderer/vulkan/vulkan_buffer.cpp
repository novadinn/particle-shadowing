#include "vulkan_buffer.h"

#include "core/logger.h"
#include "vk_check.h"
#include "vulkan_command_buffer.h"
#include "vulkan_command_pool.h"
#include "vulkan_memory_allocator.h"
#include "vulkan_queue.h"

#include <cstring>

b8 VulkanBuffer::create(VulkanMemoryAllocator *allocator, u32 buffer_size,
                        VkBufferUsageFlags usage_flags,
                        VkMemoryPropertyFlags memory_flags,
                        VmaMemoryUsage vma_usage) {
  size = buffer_size;

  VkBufferCreateInfo buffer_create_info = {};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.pNext = 0;
  buffer_create_info.flags = 0;
  buffer_create_info.size = size;
  buffer_create_info.usage = usage_flags;
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_create_info.queueFamilyIndexCount = 0;
  buffer_create_info.pQueueFamilyIndices = 0;

  VmaAllocationCreateInfo vma_allocation_create_info = {};
  /* vma_allocation_create_info.flags; */
  vma_allocation_create_info.usage = vma_usage;
  vma_allocation_create_info.requiredFlags = memory_flags;
  /*vma_allocation_create_info.preferredFlags;
  vma_allocation_create_info.memoryTypeBits;
  vma_allocation_create_info.pool;
  vma_allocation_create_info.pUserData;
  vma_allocation_create_info.priority; */

  VK_CHECK(vmaCreateBuffer(allocator->handle, &buffer_create_info,
                           &vma_allocation_create_info, &handle, &memory, 0));

  return true;
}

void VulkanBuffer::destroy(VulkanMemoryAllocator *allocator) {
  vmaDestroyBuffer(allocator->handle, handle, memory);
}

void *VulkanBuffer::lock(VulkanMemoryAllocator *allocator) {
  void *data;
  VK_CHECK(vmaMapMemory(allocator->handle, memory, &data));

  return data;
}

void VulkanBuffer::unlock(VulkanMemoryAllocator *allocator) {
  vmaUnmapMemory(allocator->handle, memory);
}

b8 VulkanBuffer::loadData(VulkanMemoryAllocator *allocator, void *data) {
  void *data_ptr = lock(allocator);
  memcpy(data_ptr, data, size);
  unlock(allocator);

  return true;
}

bool VulkanBuffer::loadDataStaging(VulkanDevice *device,
                                   VulkanMemoryAllocator *allocator, void *data,
                                   VulkanQueue *queue,
                                   VulkanCommandPool *command_pool) {
  VulkanBuffer staging_buffer;
  if (!staging_buffer.create(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             VMA_MEMORY_USAGE_CPU_ONLY)) {
    ERROR("Failed to create a staging buffer!");
    return false;
  }

  if (!staging_buffer.loadData(allocator, data)) {
    ERROR("Failed to load buffer data!");
    return false;
  }

  staging_buffer.copyTo(device, this, queue, command_pool);

  staging_buffer.destroy(allocator);

  return true;
}

bool VulkanBuffer::copyTo(VulkanDevice *device, VulkanBuffer *dest,
                          VulkanQueue *queue, VulkanCommandPool *command_pool) {
  queue->waitIdle();

  VulkanCommandBuffer temp_command_buffer;
  temp_command_buffer.allocateAndBeginSingleUse(device, command_pool);

  VkBufferCopy copy_region = {};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = 0;
  copy_region.size = size;

  /* TODO: move to command buffer struct */
  vkCmdCopyBuffer(temp_command_buffer.handle, handle, dest->handle, 1,
                  &copy_region);

  temp_command_buffer.endAndFreeSingleUse(device, command_pool, queue);

  return true;
}