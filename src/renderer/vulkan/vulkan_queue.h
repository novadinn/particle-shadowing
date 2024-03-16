#pragma once

#include "core/platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"

#include <vulkan/vulkan.h>

struct VulkanCommandBuffer;
struct VulkanSemaphore;
struct VulkanFence;

struct VulkanQueue {
  VkQueue handle;

  b8 get(VulkanDevice *device, u32 family_index);
  VkResult submit(VulkanCommandBuffer *command_buffer, u32 wait_semaphore_count,
                  VulkanSemaphore *wait_semaphores, u32 signal_semaphore_count,
                  VulkanSemaphore *signal_semaphores, VulkanFence *fence,
                  VkPipelineStageFlags *flags);
  VkResult present(VulkanSwapchain *swapchain, VulkanSemaphore *wait_semaphore,
                   u32 image_index);

  void waitIdle();
};