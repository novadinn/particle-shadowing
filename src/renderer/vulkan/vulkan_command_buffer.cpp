#include "vulkan_command_buffer.h"

#include "core/logger.h"
#include "vk_check.h"
#include "vulkan_buffer.h"
#include "vulkan_command_pool.h"
#include "vulkan_queue.h"

b8 VulkanCommandBuffer::allocate(VulkanDevice *device,
                                 VulkanCommandPool *command_pool) {
  /* TODO: move to the command pool */
  VkCommandBufferAllocateInfo handle_allocate_info = {};
  handle_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  handle_allocate_info.pNext = 0;
  handle_allocate_info.commandPool = command_pool->handle;
  handle_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  handle_allocate_info.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(device->logical_device,
                                    &handle_allocate_info, &handle));

  return true;
}

void VulkanCommandBuffer::free(VulkanDevice *device,
                               VulkanCommandPool *command_pool) {
  vkFreeCommandBuffers(device->logical_device, command_pool->handle, 1,
                       &handle);
}

b8 VulkanCommandBuffer::allocateAndBeginSingleUse(
    VulkanDevice *device, VulkanCommandPool *command_pool) {
  if (!allocate(device, command_pool)) {
    ERROR("Failed to allocate a command buffer!");
    return false;
  }
  begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  return true;
}

void VulkanCommandBuffer::endAndFreeSingleUse(VulkanDevice *device,
                                              VulkanCommandPool *command_pool,
                                              VulkanQueue *queue) {
  end();

  queue->submit(this, 0, 0, 0, 0, 0, 0);
  queue->waitIdle();

  vkFreeCommandBuffers(device->logical_device, command_pool->handle, 1,
                       &handle);
}

void VulkanCommandBuffer::begin(VkCommandBufferUsageFlags usage_flags) {
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = 0;
  begin_info.flags = usage_flags;
  begin_info.pInheritanceInfo = 0;

  VK_CHECK(vkBeginCommandBuffer(handle, &begin_info));
}

void VulkanCommandBuffer::end() { VK_CHECK(vkEndCommandBuffer(handle)); }

void VulkanCommandBuffer::renderPassBegin(VulkanRenderPass *render_pass,
                                          VulkanFramebuffer *framebuffer,
                                          glm::vec4 clear_color,
                                          glm::vec4 render_area) {
  VkClearValue clear_value = {};
  clear_value.color.float32[0] = clear_color.r;
  clear_value.color.float32[1] = clear_color.g;
  clear_value.color.float32[2] = clear_color.b;
  clear_value.color.float32[3] = clear_color.a;

  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.pNext = 0;
  render_pass_begin_info.renderPass = render_pass->handle;
  render_pass_begin_info.framebuffer = framebuffer->handle;
  render_pass_begin_info.renderArea.offset.x = render_area.x;
  render_pass_begin_info.renderArea.offset.y = render_area.y;
  render_pass_begin_info.renderArea.extent.width = render_area.z;
  render_pass_begin_info.renderArea.extent.height = render_area.w;
  render_pass_begin_info.clearValueCount = 1;
  render_pass_begin_info.pClearValues = &clear_value;

  vkCmdBeginRenderPass(handle, &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::renderPassEnd() { vkCmdEndRenderPass(handle); }

void VulkanCommandBuffer::viewportSet(glm::vec4 viewport_values) {
  VkViewport viewport;
  viewport.x = viewport_values.x;
  viewport.y = viewport_values.y;
  viewport.width = viewport_values.z;
  viewport.height = viewport_values.w;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(handle, 0, 1, &viewport);
}

void VulkanCommandBuffer::scissorSet(glm::vec4 scissor_values) {
  VkRect2D scissor;
  scissor.offset.x = scissor_values.x;
  scissor.offset.y = scissor_values.y;
  scissor.extent.width = scissor_values.z;
  scissor.extent.height = scissor_values.w;

  vkCmdSetScissor(handle, 0, 1, &scissor);
}

void VulkanCommandBuffer::pipelineBind(VkPipelineBindPoint bind_point,
                                       VulkanPipeline *pipeline) {
  vkCmdBindPipeline(handle, bind_point, pipeline->handle);
}

void VulkanCommandBuffer::draw(u32 vertex_count, u32 instance_count) {
  vkCmdDraw(handle, vertex_count, instance_count, 0, 0);
}

void VulkanCommandBuffer::dispatch(u32 local_size_x, u32 local_size_y) {
  vkCmdDispatch(handle, local_size_x, local_size_y, 1);
}

void VulkanCommandBuffer::descriptorSetBind(VulkanPipeline *pipeline,
                                            VkPipelineBindPoint bind_point,
                                            VkDescriptorSet descriptor_set,
                                            u32 set_index, u32 dynamic_offset) {
  vkCmdBindDescriptorSets(handle, bind_point, pipeline->layout, set_index, 1,
                          &descriptor_set, 1, &dynamic_offset);
}

void VulkanCommandBuffer::bufferBind(VulkanBuffer *buffer, u32 offset) {
  VkDeviceSize offsets[] = {offset};

  vkCmdBindVertexBuffers(handle, 0, 1, &buffer->handle, offsets);
}

void VulkanCommandBuffer::pushConstants(VulkanPipeline *pipeline,
                                        VkShaderStageFlags stage_flags,
                                        u32 offset, u32 size, void *values) {
  vkCmdPushConstants(handle, pipeline->layout, stage_flags, offset, size,
                     values);
}