#include "vulkan_queue.h"

#include "vk_check.h"
#include "vulkan_command_buffer.h"
#include "vulkan_fence.h"
#include "vulkan_semaphore.h"

b8 VulkanQueue::get(VulkanDevice *device, u32 family_index) {
  vkGetDeviceQueue(device->logical_device, family_index, 0, &handle);

  return true;
}

VkResult VulkanQueue::submit(VulkanCommandBuffer *command_buffer,
                             u32 wait_semaphore_count,
                             VulkanSemaphore *wait_semaphores,
                             u32 signal_semaphore_count,
                             VulkanSemaphore *signal_semaphores,
                             VulkanFence *fence, VkPipelineStageFlags *flags) {
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = 0;
  submit_info.waitSemaphoreCount = wait_semaphore_count;
  submit_info.pWaitSemaphores = wait_semaphores ? &wait_semaphores->handle : 0;
  submit_info.pWaitDstStageMask = 0;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer->handle;
  submit_info.signalSemaphoreCount = signal_semaphore_count;
  submit_info.pSignalSemaphores =
      signal_semaphores ? &signal_semaphores->handle : 0;
  submit_info.pWaitDstStageMask = flags;

  return vkQueueSubmit(handle, 1, &submit_info, fence ? fence->handle : 0);
}

VkResult VulkanQueue::present(VulkanSwapchain *swapchain,
                              VulkanSemaphore *wait_semaphore,
                              u32 image_index) {
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = 0;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = wait_semaphore ? &wait_semaphore->handle : 0;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain->handle;
  present_info.pImageIndices = &image_index;
  present_info.pResults = 0;

  return vkQueuePresentKHR(handle, &present_info);
}

void VulkanQueue::waitIdle() { VK_CHECK(vkQueueWaitIdle(handle)); }