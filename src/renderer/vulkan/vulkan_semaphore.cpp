#include "vulkan_semaphore.h"

#include "vk_check.h"

b8 VulkanSemaphore::create(VulkanDevice *device) {
  VkSemaphoreCreateInfo semaphore_create_info = {};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_create_info.pNext = 0;
  semaphore_create_info.flags = 0;

  VK_CHECK(vkCreateSemaphore(device->logical_device, &semaphore_create_info, 0,
                             &handle));

  return true;
}

void VulkanSemaphore::destroy(VulkanDevice *device) {
  vkDestroySemaphore(device->logical_device, handle, 0);
}