#pragma once

#include "core/platform.h"
#include "vulkan_device.h"
#include "vulkan_framebuffer.h"
#include "vulkan_pipeline.h"
#include "vulkan_render_pass.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct VulkanBuffer;
struct VulkanCommandPool;
struct VulkanQueue;

struct VulkanCommandBuffer {
  VkCommandBuffer handle;

  b8 allocate(VulkanDevice *device, VulkanCommandPool *command_pool);
  void free(VulkanDevice *device, VulkanCommandPool *command_pool);

  b8 allocateAndBeginSingleUse(VulkanDevice *device,
                               VulkanCommandPool *command_pool);
  void endAndFreeSingleUse(VulkanDevice *device,
                           VulkanCommandPool *command_pool, VulkanQueue *queue);

  void begin(VkCommandBufferUsageFlags usage_flags);
  void end();

  void renderPassBegin(VulkanRenderPass *render_pass,
                       VulkanFramebuffer *framebuffer, glm::vec4 clear_color,
                       glm::vec4 render_area);
  void renderPassEnd();
  void viewportSet(glm::vec4 viewport_values);
  void scissorSet(glm::vec4 scissor_values);
  void pipelineBind(VkPipelineBindPoint bind_point, VulkanPipeline *pipeline);
  void draw(u32 vertex_count, u32 instance_count);
  void drawIndexed(u32 element_count);
  void dispatch(u32 local_size_x, u32 local_size_y);
  void descriptorSetBind(VulkanPipeline *pipeline,
                         VkPipelineBindPoint bind_point,
                         VkDescriptorSet descriptor_set, u32 set_index,
                         u32 dynamic_offset_count, u32 *dynamic_offsets);
  void bufferVertexBind(VulkanBuffer *buffer, u32 offset);
  void bufferIndexBind(VulkanBuffer *buffer, u32 offset);
  void pushConstants(VulkanPipeline *pipeline, VkShaderStageFlags stage_flags,
                     u32 offset, u32 size, void *values);
};