#include "vulkan_render_pass.h"

#include "vk_check.h"

b8 VulkanRenderPass::create(VulkanDevice *device, VulkanSwapchain *swapchain) {
  /* TODO: purely hardcoded for now */

  VkAttachmentDescription attachment_description = {};
  attachment_description.flags = 0;
  attachment_description.format = swapchain->image_format.format;
  attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_reference = {};
  color_attachment_reference.attachment = 0;
  color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass_description = {};
  subpass_description.flags = 0;
  subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_description.inputAttachmentCount = 0;
  subpass_description.pInputAttachments = 0;
  subpass_description.colorAttachmentCount = 1;
  subpass_description.pColorAttachments = &color_attachment_reference;
  subpass_description.pResolveAttachments = 0;
  subpass_description.pDepthStencilAttachment = 0;
  subpass_description.preserveAttachmentCount = 0;
  subpass_description.pPreserveAttachments = 0;

  VkSubpassDependency subpass_dependency = {};
  subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency.dstSubpass = 0;
  subpass_dependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.srcAccessMask = 0;
  subpass_dependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpass_dependency.dependencyFlags = 0;

  VkRenderPassCreateInfo render_pass_create_info = {};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.pNext = 0;
  render_pass_create_info.flags = 0;
  render_pass_create_info.attachmentCount = 1;
  render_pass_create_info.pAttachments = &attachment_description;
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass_description;
  render_pass_create_info.dependencyCount = 1;
  render_pass_create_info.pDependencies = &subpass_dependency;

  VK_CHECK(vkCreateRenderPass(device->logical_device, &render_pass_create_info,
                              0, &handle));

  return true;
}

void VulkanRenderPass::destroy(VulkanDevice *device) {
  vkDestroyRenderPass(device->logical_device, handle, 0);
}