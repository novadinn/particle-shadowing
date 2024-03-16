#include "vulkan_descriptor_set_builder.h"

#include "core/logger.h"
#include "vulkan_buffer.h"
#include "vulkan_descriptor_allocator.h"
#include "vulkan_descriptor_set_layout_cache.h"
#include "vulkan_texture.h"

b8 VulkanDescriptorSetBuilder::begin() { return true; }

void VulkanDescriptorSetBuilder::bufferBind(u32 binding,
                                            VkDescriptorBufferInfo *buffer_info,
                                            VkDescriptorType type,
                                            VkShaderStageFlags stage_flags) {
  VkDescriptorSetLayoutBinding layout_binding = {};
  layout_binding.binding = binding;
  layout_binding.descriptorType = type;
  layout_binding.descriptorCount = 1;
  layout_binding.stageFlags = stage_flags;
  layout_binding.pImmutableSamplers = nullptr;

  bindings.emplace_back(layout_binding);

  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.pNext = nullptr;
  /* write_descriptor_set.dstSet; set later */
  write_descriptor_set.dstBinding = binding;
  write_descriptor_set.dstArrayElement = 0;
  write_descriptor_set.descriptorCount = 1;
  write_descriptor_set.descriptorType = type;
  write_descriptor_set.pImageInfo = 0;
  write_descriptor_set.pBufferInfo = buffer_info;
  write_descriptor_set.pTexelBufferView = 0;

  writes.emplace_back(write_descriptor_set);
}

void VulkanDescriptorSetBuilder::imageBind(u32 binding,
                                           VkDescriptorImageInfo *image_info,
                                           VkDescriptorType type,
                                           VkShaderStageFlags stage_flags) {
  VkDescriptorSetLayoutBinding layout_binding = {};
  layout_binding.binding = binding;
  layout_binding.descriptorType = type;
  layout_binding.descriptorCount = 1;
  layout_binding.stageFlags = stage_flags;
  layout_binding.pImmutableSamplers = nullptr;

  bindings.emplace_back(layout_binding);

  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.pNext = nullptr;
  /* write_descriptor_set.dstSet; set later */
  write_descriptor_set.dstBinding = binding;
  write_descriptor_set.dstArrayElement = 0;
  write_descriptor_set.descriptorCount = 1;
  write_descriptor_set.descriptorType = type;
  write_descriptor_set.pImageInfo = image_info;
  write_descriptor_set.pBufferInfo = 0;
  write_descriptor_set.pTexelBufferView = 0;

  writes.emplace_back(write_descriptor_set);
}

b8 VulkanDescriptorSetBuilder::end(VulkanDevice *device,
                                   VkDescriptorSet *out_set,
                                   VkDescriptorSetLayout *out_layout) {
  VkDescriptorSetLayoutCreateInfo layout_info = {};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.pNext = 0;
  layout_info.flags = 0;
  layout_info.pBindings = bindings.data();
  layout_info.bindingCount = bindings.size();

  *out_layout =
      VulkanDescriptorSetLayoutCache::layoutCreate(device, &layout_info);

  if (!VulkanDescriptorAllocator::allocateDescriptorSet(device, *out_layout,
                                                        out_set)) {
    ERROR("Failed to allocate a descriptor set!");
    return false;
  }

  for (VkWriteDescriptorSet &w : writes) {
    w.dstSet = *out_set;
  }

  vkUpdateDescriptorSets(device->logical_device, writes.size(), writes.data(),
                         0, 0);

  return true;
}

b8 VulkanDescriptorSetBuilder::end(VulkanDevice *device,
                                   VkDescriptorSet *out_set) {
  VkDescriptorSetLayout layout;
  return end(device, out_set, &layout);
}

VkDescriptorSetLayoutBinding
vulkanDescriptorSetLayoutBinding(u32 binding, VkDescriptorType descriptor_type,
                                 VkShaderStageFlags shader_stage_flags) {
  VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
  descriptor_set_layout_binding.binding = binding;
  descriptor_set_layout_binding.descriptorType = descriptor_type;
  descriptor_set_layout_binding.descriptorCount = 1;
  descriptor_set_layout_binding.stageFlags = shader_stage_flags;
  descriptor_set_layout_binding.pImmutableSamplers = 0;

  return descriptor_set_layout_binding;
}

VkDescriptorImageInfo vulkanDescriptorImageInfo(VulkanTexture *texture,
                                                VkImageLayout layout) {
  VkDescriptorImageInfo descriptor_image_info = {};
  descriptor_image_info.sampler = texture->sampler;
  descriptor_image_info.imageView = texture->view;
  descriptor_image_info.imageLayout = layout;

  return descriptor_image_info;
}

VkDescriptorBufferInfo vulkanDescriptorBufferInfo(VulkanBuffer *buffer) {
  VkDescriptorBufferInfo descriptor_buffer_info = {};
  descriptor_buffer_info.buffer = buffer->handle;
  descriptor_buffer_info.offset = 0;
  descriptor_buffer_info.range = buffer->size;

  return descriptor_buffer_info;
}