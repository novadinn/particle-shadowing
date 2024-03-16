#include "vulkan_framebuffer.h"

#include "vk_check.h"

b8 VulkanFramebuffer::create(VulkanDevice *device,
                             VulkanRenderPass *render_pass,
                             std::vector<VkImageView> &attachments, u32 width,
                             u32 height) {
  VkFramebufferCreateInfo framebuffer_create_info = {};
  framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_create_info.pNext = 0;
  framebuffer_create_info.flags = 0;
  framebuffer_create_info.renderPass = render_pass->handle;
  framebuffer_create_info.attachmentCount = attachments.size();
  framebuffer_create_info.pAttachments = attachments.data();
  framebuffer_create_info.width = width;
  framebuffer_create_info.height = height;
  framebuffer_create_info.layers = 1;

  VK_CHECK(vkCreateFramebuffer(device->logical_device, &framebuffer_create_info,
                               0, &handle));

  return true;
}

void VulkanFramebuffer::destroy(VulkanDevice *device) {
  vkDestroyFramebuffer(device->logical_device, handle, 0);
}