#pragma once

#include "vulkan_device.h"
#include "vulkan_render_pass.h"

#include <vector>
#include <vulkan/vulkan.h>

struct VulkanShaderModule;

struct VulkanPipeline {
  VkPipeline handle;
  VkPipelineLayout layout;

  b8 createGraphics(VulkanDevice *device, VulkanRenderPass *render_pass,
                    u32 descriptor_set_layout_count,
                    VkDescriptorSetLayout *descriptor_set_layouts,
                    u32 stage_info_count,
                    VkPipelineShaderStageCreateInfo *stage_infos,
                    u32 push_constants_count,
                    VkPushConstantRange *push_constants,
                    u32 dynamic_state_count, VkDynamicState *dynamic_states,
                    VkViewport viewport, VkRect2D scissor);
  b8 createCompute(VulkanDevice *device, u32 descriptor_set_layout_count,
                   VkDescriptorSetLayout *descriptor_set_layouts,
                   VkPipelineShaderStageCreateInfo stage);
  void destroy(VulkanDevice *device);
};

VkPipelineShaderStageCreateInfo
vulkanPipelineShaderStageCreateInfo(VulkanShaderModule *shader_module,
                                    VkShaderStageFlagBits stage_flag);