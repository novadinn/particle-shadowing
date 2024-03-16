#include "vulkan_fence.h"

#include "vk_check.h"

b8 VulkanFence::create(VulkanDevice *device) {
  VkFenceCreateInfo fence_create_info = {};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.pNext = 0;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VK_CHECK(
      vkCreateFence(device->logical_device, &fence_create_info, 0, &handle));

  return true;
}

void VulkanFence::destroy(VulkanDevice *device) {
  vkDestroyFence(device->logical_device, handle, 0);
}

void VulkanFence::wait(VulkanDevice *device, u64 timeout) {
  vkWaitForFences(device->logical_device, 1, &handle, true, timeout);
}

void VulkanFence::reset(VulkanDevice *device) {
  VK_CHECK(vkResetFences(device->logical_device, 1, &handle));
}