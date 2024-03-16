#include "vulkan_memory_allocator.h"

#include "vk_check.h"
#include "vulkan_instance.h"

b8 VulkanMemoryAllocator::create(VulkanInstance *instance, VulkanDevice *device,
                                 u32 api_version) {
  VmaAllocatorCreateInfo vma_allocator_create_info = {};
  vma_allocator_create_info.flags = 0;
  vma_allocator_create_info.physicalDevice = device->physical_device;
  vma_allocator_create_info.device = device->logical_device;
  /* vma_allocator_create_info.preferredLargeHeapBlockSize; */
  vma_allocator_create_info.pAllocationCallbacks = 0;
  /* vma_allocator_create_info.pDeviceMemoryCallbacks; */
  /* vma_allocator_create_info.pHeapSizeLimit; */
  /* vma_allocator_create_info.pVulkanFunctions; */
  vma_allocator_create_info.instance = instance->handle;
  vma_allocator_create_info.vulkanApiVersion = api_version;

  VK_CHECK(vmaCreateAllocator(&vma_allocator_create_info, &handle));

  return true;
}

void VulkanMemoryAllocator::destroy() { vmaDestroyAllocator(handle); }