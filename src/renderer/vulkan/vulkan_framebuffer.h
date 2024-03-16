#pragma once

#include "core/platform.h"
#include "vulkan_device.h"
#include "vulkan_render_pass.h"

#include <vector>
#include <vulkan/vulkan.h>

struct VulkanFramebuffer {
  VkFramebuffer handle;

  b8 create(VulkanDevice *device, VulkanRenderPass *render_pass,
            std::vector<VkImageView> &attachments, u32 width, u32 height);
  void destroy(VulkanDevice *device);
};