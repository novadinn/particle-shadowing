#include "vulkan_shader_module.h"

#include "core/logger.h"
#include "vk_check.h"

#include <stdio.h>

b8 VulkanShaderModule::create(VulkanDevice *device, const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    ERROR("Failed to open file %s", path);
    return false;
  }

  fseek(file, 0, SEEK_END);
  i64 file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  std::vector<u32> file_data;
  file_data.resize(file_size);
  fread(&file_data[0], file_size, 1, file);
  fclose(file);

  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = 0;
  create_info.flags = 0;
  create_info.codeSize = file_size;
  create_info.pCode = &file_data[0];

  VK_CHECK(
      vkCreateShaderModule(device->logical_device, &create_info, 0, &handle));

  return true;
}

void VulkanShaderModule::destroy(VulkanDevice *device) {
  vkDestroyShaderModule(device->logical_device, handle, 0);
}