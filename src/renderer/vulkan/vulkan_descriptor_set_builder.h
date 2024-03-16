#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vector>
#include <vulkan/vulkan.h>

struct VulkanTexture;
struct VulkanBuffer;

struct VulkanDescriptorSetBuilder {
  std::vector<VkWriteDescriptorSet> writes;
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  b8 begin();
  void bufferBind(uint32_t binding, VkDescriptorBufferInfo *buffer_info,
                  VkDescriptorType type, VkShaderStageFlags stage_flags);
  void imageBind(uint32_t binding, VkDescriptorImageInfo *image_info,
                 VkDescriptorType type, VkShaderStageFlags stage_flags);
  b8 end(VulkanDevice *device, VkDescriptorSet *out_set,
         VkDescriptorSetLayout *out_layout);
  b8 end(VulkanDevice *device, VkDescriptorSet *out_set);
};

VkDescriptorSetLayoutBinding
vulkanDescriptorSetLayoutBinding(u32 binding, VkDescriptorType descriptor_type,
                                 VkShaderStageFlags shader_stage_flags);
VkDescriptorImageInfo vulkanDescriptorImageInfo(VulkanTexture *texture,
                                                VkImageLayout layout);
VkDescriptorBufferInfo vulkanDescriptorBufferInfo(VulkanBuffer *buffer);