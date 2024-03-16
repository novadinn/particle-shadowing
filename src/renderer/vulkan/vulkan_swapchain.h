#pragma once

#include "core/platform.h"
#include "vulkan_device.h"

#include <vector>
#include <vulkan/vulkan.h>

struct VulkanSurface;
struct VulkanSemaphore;

struct VulkanSwapchain {
  VkSwapchainKHR handle;
  u32 max_frames_in_flight;
  std::vector<VkImage> images;
  std::vector<VkImageView> image_views;
  VkSurfaceFormatKHR image_format;

  b8 create(VulkanDevice *device, VulkanSurface *surface, u32 width,
            u32 height);
  void destroy(VulkanDevice *device);

  void acquireNextImageIndex(VulkanDevice *device, u64 timeout,
                             VulkanSemaphore *semaphore, u32 *out_image_index);
};