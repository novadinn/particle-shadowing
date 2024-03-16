#include "vulkan_surface.h"

#include "vulkan_instance.h"

#include <SDL_vulkan.h>

b8 VulkanSurface::create(VulkanInstance *instance, SDL_Window *window) {
  return SDL_Vulkan_CreateSurface(window, instance->handle, &handle);
}

void VulkanSurface::destroy(VulkanInstance *instance) {
  vkDestroySurfaceKHR(instance->handle, handle, 0);
}