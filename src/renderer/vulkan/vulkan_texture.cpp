#include "vulkan_texture.h"

#include "core/logger.h"
#include "vk_check.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_command_pool.h"
#include "vulkan_memory_allocator.h"
#include "vulkan_queue.h"

b8 VulkanTexture::create(VulkanDevice *device, VulkanMemoryAllocator *allocator,
                         VkFormat texture_format, u32 texture_width,
                         u32 texture_height, VkImageUsageFlags usage_flags) {
  if (usage_flags & VK_IMAGE_USAGE_STORAGE_BIT) {
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(device->physical_device, format,
                                        &format_properties);

    if (!format_properties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
      ERROR("Device does not support VK_IMAGE_USAGE_STORAGE_BIT!");
      return false;
    }
  }

  format = texture_format;
  width = texture_width;
  height = texture_height;

  VkImageCreateInfo image_create_info = {};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = 0;
  image_create_info.flags = 0;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = format;
  image_create_info.extent.width = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage = usage_flags;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.queueFamilyIndexCount = 0;
  image_create_info.pQueueFamilyIndices = 0;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VmaAllocationCreateInfo vma_allocation_create_info = {};
  /* vma_allocation_create_info.flags; */
  vma_allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  vma_allocation_create_info.requiredFlags =
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  /* vma_allocation_create_info.preferredFlags;
  vma_allocation_create_info.memoryTypeBits;
  vma_allocation_create_info.pool;
  vma_allocation_create_info.pUserData;
  vma_allocation_create_info.priority; */

  /* TODO: move all vma functions to the vma */
  VK_CHECK(vmaCreateImage(allocator->handle, &image_create_info,
                          &vma_allocation_create_info, &handle, &memory, 0));

  VkImageViewCreateInfo view_create_info = {};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = 0;
  view_create_info.flags = 0;
  view_create_info.image = handle;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = format;
  /* view_create_info.components; */
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;

  VK_CHECK(
      vkCreateImageView(device->logical_device, &view_create_info, 0, &view));

  VkSamplerCreateInfo sampler_create_info = {};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.pNext = 0;
  sampler_create_info.flags = 0;
  sampler_create_info.magFilter = VK_FILTER_LINEAR;
  sampler_create_info.minFilter = VK_FILTER_LINEAR;
  sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.mipLodBias = 0.0f;
  sampler_create_info.anisotropyEnable = VK_FALSE;
  sampler_create_info.maxAnisotropy = 1.0f;
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
  sampler_create_info.minLod = 0.0f;
  sampler_create_info.maxLod = (float)1;
  sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;

  VK_CHECK(vkCreateSampler(device->logical_device, &sampler_create_info, 0,
                           &sampler));

  return true;
}

void VulkanTexture::destroy(VulkanDevice *device,
                            VulkanMemoryAllocator *allocator) {
  vkDestroySampler(device->logical_device, sampler, 0);
  vkDestroyImageView(device->logical_device, view, 0);
  vmaDestroyImage(allocator->handle, handle, memory);
}

b8 VulkanTexture::writeData(VulkanDevice *device, void *pixels,
                            VulkanMemoryAllocator *allocator,
                            VulkanQueue *queue, VulkanCommandPool *command_pool,
                            u32 queue_family_index) {
  /* TODO: instead of 4, determine the size of the texture via texture->format
   */
  u32 size = width * height * 4;

  VulkanBuffer staging_buffer;
  if (!staging_buffer.create(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             VMA_MEMORY_USAGE_CPU_ONLY)) {
    ERROR("Failed to create a staging buffer!");
    return false;
  }

  if (!staging_buffer.loadData(allocator, pixels)) {
    ERROR("Failed to load buffer data!");
    return false;
  }

  VulkanCommandBuffer temp_command_buffer;
  if (!temp_command_buffer.allocateAndBeginSingleUse(device, command_pool)) {
    ERROR("Failed to allocate a temp command buffer!");
    return false;
  }

  if (!transitionLayout(&temp_command_buffer, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        queue_family_index)) {
    ERROR("Failed to transition image layout!");
    return false;
  }
  if (!copyFromBuffer(&staging_buffer, &temp_command_buffer)) {
    ERROR("Failed to copy buffer data to texture!");
    return false;
  }
  if (!transitionLayout(
          &temp_command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, queue_family_index)) {
    ERROR("Failed to transition image layout!");
    return false;
  }

  temp_command_buffer.endAndFreeSingleUse(device, command_pool, queue);
  staging_buffer.destroy(allocator);

  return true;
}

b8 VulkanTexture::transitionLayout(VulkanCommandBuffer *command_buffer,
                                   VkImageLayout old_layout,
                                   VkImageLayout new_layout,
                                   u32 queue_family_index) {
  VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = queue_family_index;
  barrier.dstQueueFamilyIndex = queue_family_index;
  barrier.image = handle;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags dest_stage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_GENERAL) {
    barrier.srcAccessMask = 0;
    source_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    dest_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_GENERAL) {
    barrier.srcAccessMask = 0;
    source_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    dest_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  } else {
    ERROR("Unsupported layout transition!");
    return false;
  }

  /* TODO: move to the command buffer */
  vkCmdPipelineBarrier(command_buffer->handle, source_stage, dest_stage, 0, 0,
                       0, 0, 0, 1, &barrier);

  return true;
}

b8 VulkanTexture::copyFromBuffer(VulkanBuffer *buffer,
                                 VulkanCommandBuffer *command_buffer) {
  /* TODO: instead of 4, determine the size of the texture via texture->format
   */
  u32 size = width * height * 4;

  VkBufferImageCopy buffer_image_copy = {};
  buffer_image_copy.bufferOffset = 0;
  buffer_image_copy.bufferRowLength = 0;
  buffer_image_copy.bufferImageHeight = 0;
  buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  buffer_image_copy.imageSubresource.mipLevel = 0;
  buffer_image_copy.imageSubresource.baseArrayLayer = 0;
  buffer_image_copy.imageSubresource.layerCount = 1;
  buffer_image_copy.imageExtent.width = width;
  buffer_image_copy.imageExtent.height = height;
  buffer_image_copy.imageExtent.depth = 1;

  /* TODO: move to the command buffer */
  vkCmdCopyBufferToImage(command_buffer->handle, buffer->handle, handle,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &buffer_image_copy);

  return true;
}