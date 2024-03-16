#pragma once

#include "core/platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

#include <vulkan/vulkan.h>

struct VulkanRenderPass {
  VkRenderPass handle;

  b8 create(VulkanDevice *device, VulkanSwapchain *swapchain);
  void destroy(VulkanDevice *device);
};