#include "vulkan_device.h"

#include "core/logger.h"
#include "vk_check.h"
#include "vulkan_instance.h"
#include "vulkan_surface.h"

#include <cstring>
#include <set>

static b8
deviceExtensionsAvailable(VkPhysicalDevice physical_device,
                          std::vector<const char *> &required_extensions);

b8 VulkanDevice::create(VulkanInstance *instance, VulkanSurface *surface) {
  std::vector<VkPhysicalDevice> physical_devices;
  u32 physical_device_count = 0;
  VK_CHECK(
      vkEnumeratePhysicalDevices(instance->handle, &physical_device_count, 0));
  if (physical_device_count == 0) {
    ERROR("Failed to find GPU with Vulkan support!");
    return false;
  }
  physical_devices.resize(physical_device_count);
  VK_CHECK(vkEnumeratePhysicalDevices(instance->handle, &physical_device_count,
                                      physical_devices.data()));

  for (u32 i = 0; i < physical_devices.size(); ++i) {
    VkPhysicalDevice current_physical_device = physical_devices[i];

    std::vector<const char *> device_extension_names;
    device_extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef PLATFORM_APPLE
    device_extension_names.emplace_back("VK_KHR_portability_subset");
#endif

    if (!deviceExtensionsAvailable(current_physical_device,
                                   device_extension_names)) {
      return false;
    }

    std::vector<VkQueueFamilyProperties> queue_family_properties;
    u32 queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(current_physical_device,
                                             &queue_family_count, 0);
    queue_family_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(current_physical_device,
                                             &queue_family_count,
                                             &queue_family_properties[0]);

    i32 device_graphics_family_index = -1;
    i32 device_present_family_index = -1;
    i32 device_compute_family_index = -1;
    i32 device_transfer_family_index = -1;
    for (u32 j = 0; j < queue_family_count; ++j) {
      VkQueueFamilyProperties queue_properties = queue_family_properties[j];

      if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        device_graphics_family_index = j;

        VkBool32 supports_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(
            current_physical_device, j, surface->handle, &supports_present));
        if (supports_present) {
          device_present_family_index = j;
        }
      }

      if (queue_properties.queueFlags & VK_QUEUE_TRANSFER_BIT) {
        device_transfer_family_index = j;
      }

      if (queue_properties.queueFlags & VK_QUEUE_COMPUTE_BIT) {
        device_compute_family_index = j;
      }

      /* attempting to find a transfer-only queue (can be used for multithreaded
       * transfer operations) */
      for (u32 k = 0; k < queue_family_count; ++k) {
        VkQueueFamilyProperties queue_properties = queue_family_properties[k];

        if ((queue_properties.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(queue_properties.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            !(queue_properties.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(queue_properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) &&
            !(queue_properties.queueFlags & VK_QUEUE_PROTECTED_BIT) &&
            !(queue_properties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) &&
            !(queue_properties.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)) {
          device_transfer_family_index = k;
        }
      }
    }

    if (device_graphics_family_index == -1 ||
        device_present_family_index == -1 ||
        device_transfer_family_index == -1 ||
        device_compute_family_index == -1) {
      return false;
    }

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(current_physical_device, &device_properties);
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(current_physical_device, &device_features);
    VkPhysicalDeviceMemoryProperties device_memory;
    vkGetPhysicalDeviceMemoryProperties(current_physical_device,
                                        &device_memory);

    physical_device = current_physical_device;
    properties = device_properties;
    features = device_features;
    memory = device_memory;
    graphics_family_index = device_graphics_family_index;
    present_family_index = device_present_family_index;
    compute_family_index = device_compute_family_index;
    transfer_family_index = device_transfer_family_index;

    break;
  }

  std::vector<u32> queue_indices;
  std::set<u32> unique_queue_indices;
  if (!unique_queue_indices.count(graphics_family_index)) {
    queue_indices.emplace_back(graphics_family_index);
  }
  unique_queue_indices.emplace(graphics_family_index);

  if (!unique_queue_indices.count(present_family_index)) {
    queue_indices.emplace_back(present_family_index);
  }
  unique_queue_indices.emplace(present_family_index);

  if (!unique_queue_indices.count(compute_family_index)) {
    queue_indices.emplace_back(compute_family_index);
  }
  unique_queue_indices.emplace(compute_family_index);

  if (!unique_queue_indices.count(transfer_family_index)) {
    queue_indices.emplace_back(transfer_family_index);
  }
  unique_queue_indices.emplace(transfer_family_index);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  for (u32 i = 0; i < queue_indices.size(); ++i) {
    f32 queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pNext = 0;
    queue_create_info.flags = 0;
    queue_create_info.queueFamilyIndex = queue_indices[i];
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    queue_create_infos.emplace_back(queue_create_info);
  }

  std::vector<const char *> required_extension_names;
  required_extension_names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef PLATFORM_APPLE
  required_extension_names.emplace_back("VK_KHR_portability_subset");
#endif

  VkPhysicalDeviceFeatures device_features = {};

  VkDeviceCreateInfo device_create_info = {};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pNext = 0;
  device_create_info.flags = 0;
  device_create_info.queueCreateInfoCount = queue_create_infos.size();
  device_create_info.pQueueCreateInfos = queue_create_infos.data();
  device_create_info.enabledLayerCount = 0;   /* deprecated */
  device_create_info.ppEnabledLayerNames = 0; /* deprecated */
  device_create_info.enabledExtensionCount = required_extension_names.size();
  device_create_info.ppEnabledExtensionNames = required_extension_names.data();
  device_create_info.pEnabledFeatures = &device_features;

  VK_CHECK(
      vkCreateDevice(physical_device, &device_create_info, 0, &logical_device));

  return true;
}

void VulkanDevice::destroy() { vkDestroyDevice(logical_device, 0); }

void VulkanDevice::waitIdle() { vkDeviceWaitIdle(logical_device); }

static b8
deviceExtensionsAvailable(VkPhysicalDevice physical_device,
                          std::vector<const char *> &required_extensions) {
  u32 available_extension_count = 0;
  std::vector<VkExtensionProperties> available_extensions;

  VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, 0,
                                                &available_extension_count, 0));
  if (available_extension_count != 0) {
    available_extensions.resize(available_extension_count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, 0,
                                                  &available_extension_count,
                                                  &available_extensions[0]));

    for (u32 i = 0; i < required_extensions.size(); ++i) {
      b8 found = false;
      for (u32 j = 0; j < available_extension_count; ++j) {
        if (strcmp(required_extensions[i],
                   available_extensions[j].extensionName)) {
          found = true;
          break;
        }
      }

      if (!found) {
        DEBUG("Required device extension not found: '%s', skipping device.",
              required_extensions[i]);
        return false;
      }
    }
  }

  return true;
}