#include "vulkan_command_pool.h"

#include "vk_check.h"

b8 VulkanCommandPool::create(VulkanDevice *device, u32 family_index) {
  VkCommandPoolCreateInfo command_pool_create_info = {};
  command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_create_info.pNext = 0;
  command_pool_create_info.flags =
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_create_info.queueFamilyIndex = family_index;

  VK_CHECK(vkCreateCommandPool(device->logical_device,
                               &command_pool_create_info, 0, &handle));

  return true;
}

void VulkanCommandPool::destroy(VulkanDevice *device) {
  vkDestroyCommandPool(device->logical_device, handle, 0);
}