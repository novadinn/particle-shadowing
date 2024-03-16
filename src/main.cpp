#include "core/input.h"
#include "core/logger.h"
#include "core/platform.h"

#include "renderer/vulkan/vk_check.h"
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan/vulkan_command_pool.h"
#include "renderer/vulkan/vulkan_debug_messenger.h"
#include "renderer/vulkan/vulkan_descriptor_allocator.h"
#include "renderer/vulkan/vulkan_descriptor_set_builder.h"
#include "renderer/vulkan/vulkan_descriptor_set_layout_cache.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_fence.h"
#include "renderer/vulkan/vulkan_framebuffer.h"
#include "renderer/vulkan/vulkan_instance.h"
#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include "renderer/vulkan/vulkan_memory_allocator.h"
#include "renderer/vulkan/vulkan_queue.h"
#include "renderer/vulkan/vulkan_render_pass.h"
#include "renderer/vulkan/vulkan_semaphore.h"
#include "renderer/vulkan/vulkan_shader_module.h"
#include "renderer/vulkan/vulkan_surface.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_texture.h"

#include <SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

struct Particle {
  glm::vec3 pos;
  float radius;
  float opacity;
};

struct GlobalUBO {
  glm::mat4 projection;
  glm::mat4 view;
};

struct PushConstants {
  glm::mat4 model;
};

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    FATAL("Failed to initialize SDL!");
    exit(1);
  }

  SDL_Window *window;
  const u32 window_width = 800;
  const u32 window_height = 600;

  window = SDL_CreateWindow("Particle Shadowing", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, window_width,
                            window_height, SDL_WINDOW_VULKAN);

  if (!Input::initialize()) {
    FATAL("Failed to initalize an input system!");
    exit(1);
  }

  VkApplicationInfo application_info = {};
  application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  application_info.pNext = nullptr;
  application_info.pApplicationName = "Particle Shadowing";
  application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.pEngineName = "Particle Shadowing";
  application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.apiVersion = VK_API_VERSION_1_3;

  VulkanInstance instance;
  instance.create(application_info, window);

#ifdef PLATFORM_APPLE
  setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1);
#endif

#ifndef NDEBUG
  VulkanDebugMessenger debug_messenger;
  debug_messenger.create(&instance);
#endif

  VulkanSurface surface;
  surface.create(&instance, window);

  VulkanDevice device;
  device.create(&instance, &surface);

  VulkanSwapchain swapchain;
  swapchain.create(&device, &surface, window_width, window_height);

  VulkanRenderPass render_pass;
  render_pass.create(&device, &swapchain);

  std::vector<VulkanFramebuffer> framebuffers;
  framebuffers.resize(swapchain.images.size());
  for (u32 i = 0; i < framebuffers.size(); ++i) {
    std::vector<VkImageView> attachments = {swapchain.image_views[i]};
    framebuffers[i].create(&device, &render_pass, attachments, window_width,
                           window_height);
  }

  VulkanMemoryAllocator allocator;
  allocator.create(&instance, &device, application_info.apiVersion);

  VulkanQueue graphics_queue;
  graphics_queue.get(&device, device.graphics_family_index);
  VulkanQueue compute_queue;
  compute_queue.get(&device, device.compute_family_index);
  VulkanQueue present_queue;
  present_queue.get(&device, device.present_family_index);

  VulkanCommandPool graphics_command_pool;
  graphics_command_pool.create(&device, device.graphics_family_index);
  VulkanCommandPool compute_command_pool;
  compute_command_pool.create(&device, device.compute_family_index);

  std::vector<VulkanCommandBuffer> graphics_command_buffers;
  graphics_command_buffers.resize(swapchain.images.size());
  std::vector<VulkanCommandBuffer> compute_command_buffers;
  compute_command_buffers.resize(swapchain.images.size());
  for (u32 i = 0; i < swapchain.images.size(); ++i) {
    graphics_command_buffers[i].allocate(&device, &graphics_command_pool);
    compute_command_buffers[i].allocate(&device, &compute_command_pool);
  }

  std::vector<VulkanSemaphore> image_available_semaphores;
  image_available_semaphores.resize(swapchain.max_frames_in_flight);
  std::vector<VulkanSemaphore> render_finished_semaphores;
  render_finished_semaphores.resize(swapchain.max_frames_in_flight);
  std::vector<VulkanSemaphore> compute_finished_semaphores;
  compute_finished_semaphores.resize(swapchain.max_frames_in_flight);
  std::vector<VulkanFence> in_flight_fences;
  in_flight_fences.resize(swapchain.max_frames_in_flight);
  std::vector<VulkanFence> compute_in_flight_fences;
  compute_in_flight_fences.resize(swapchain.max_frames_in_flight);

  for (u32 i = 0; i < swapchain.max_frames_in_flight; ++i) {
    image_available_semaphores[i].create(&device);
    render_finished_semaphores[i].create(&device);
    compute_finished_semaphores[i].create(&device);
    in_flight_fences[i].create(&device);
    compute_in_flight_fences[i].create(&device);
  }

  VulkanDescriptorAllocator::initialize();
  VulkanDescriptorSetLayoutCache::initialize();

  VulkanShaderModule vertex_shader_module;
  vertex_shader_module.create(&device, "assets/shaders/particle.vert.spv");
  VulkanShaderModule fragment_shader_module;
  fragment_shader_module.create(&device, "assets/shaders/particle.frag.spv");

  VkDescriptorSetLayoutBinding descriptor_set_layout_binding =
      vulkanDescriptorSetLayoutBinding(
          0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
          VK_SHADER_STAGE_VERTEX_BIT);
  VkDescriptorSetLayoutCreateInfo graphics_descriptor_set_layout_create_info =
      {};
  graphics_descriptor_set_layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  graphics_descriptor_set_layout_create_info.pNext = 0;
  graphics_descriptor_set_layout_create_info.flags = 0;
  graphics_descriptor_set_layout_create_info.bindingCount = 1;
  graphics_descriptor_set_layout_create_info.pBindings =
      &descriptor_set_layout_binding;
  VkDescriptorSetLayout graphics_descriptor_set_layout =
      VulkanDescriptorSetLayoutCache::layoutCreate(
          &device, &graphics_descriptor_set_layout_create_info);

  std::vector<VkPipelineShaderStageCreateInfo>
      graphics_pipeline_stage_create_infos;
  graphics_pipeline_stage_create_infos.resize(2);
  graphics_pipeline_stage_create_infos[0].sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  graphics_pipeline_stage_create_infos[0].pNext = 0;
  graphics_pipeline_stage_create_infos[0].flags = 0;
  graphics_pipeline_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  graphics_pipeline_stage_create_infos[0].module = vertex_shader_module.handle;
  graphics_pipeline_stage_create_infos[0].pName = "main";
  graphics_pipeline_stage_create_infos[0].pSpecializationInfo = 0;
  graphics_pipeline_stage_create_infos[1].sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  graphics_pipeline_stage_create_infos[1].pNext = 0;
  graphics_pipeline_stage_create_infos[1].flags = 0;
  graphics_pipeline_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  graphics_pipeline_stage_create_infos[1].module =
      fragment_shader_module.handle;
  graphics_pipeline_stage_create_infos[1].pName = "main";
  graphics_pipeline_stage_create_infos[1].pSpecializationInfo = 0;

  VkPushConstantRange push_constant_range = {};
  push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  push_constant_range.offset = 0;
  push_constant_range.size = sizeof(PushConstants);

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = window_height;
  viewport.width = window_width;
  viewport.height = -(i32)window_height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = window_width;
  scissor.extent.height = window_height;

  VulkanPipeline graphics_pipeline;
  graphics_pipeline.createGraphics(
      &device, &render_pass, 1, &graphics_descriptor_set_layout,
      graphics_pipeline_stage_create_infos.size(),
      graphics_pipeline_stage_create_infos.data(), 1, &push_constant_range, 0,
      0, viewport, scissor);

  vertex_shader_module.destroy(&device);
  fragment_shader_module.destroy(&device);

  VulkanShaderModule compute_shader_module;
  compute_shader_module.create(&device,
                               "assets/shaders/particle_shadowing.comp.spv");

  VkPipelineShaderStageCreateInfo compute_stage_create_info = {};
  compute_stage_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  compute_stage_create_info.pNext = 0;
  compute_stage_create_info.flags = 0;
  compute_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  compute_stage_create_info.module = compute_shader_module.handle;
  compute_stage_create_info.pName = "main";
  compute_stage_create_info.pSpecializationInfo = 0;

  descriptor_set_layout_binding = vulkanDescriptorSetLayoutBinding(
      0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
  VkDescriptorSetLayoutCreateInfo
      compute_readonly_descriptor_set_layout_create_info = {};
  compute_readonly_descriptor_set_layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  compute_readonly_descriptor_set_layout_create_info.pNext = 0;
  compute_readonly_descriptor_set_layout_create_info.flags = 0;
  compute_readonly_descriptor_set_layout_create_info.bindingCount = 1;
  compute_readonly_descriptor_set_layout_create_info.pBindings =
      &descriptor_set_layout_binding;

  descriptor_set_layout_binding = vulkanDescriptorSetLayoutBinding(
      0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
  VkDescriptorSetLayoutCreateInfo
      compute_writeonly_descriptor_set_layout_create_info = {};
  compute_writeonly_descriptor_set_layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  compute_writeonly_descriptor_set_layout_create_info.pNext = 0;
  compute_writeonly_descriptor_set_layout_create_info.flags = 0;
  compute_writeonly_descriptor_set_layout_create_info.bindingCount = 1;
  compute_writeonly_descriptor_set_layout_create_info.pBindings =
      &descriptor_set_layout_binding;

  std::vector<VkDescriptorSetLayout> compute_descriptor_set_layouts;
  compute_descriptor_set_layouts.resize(2);
  compute_descriptor_set_layouts[0] =
      VulkanDescriptorSetLayoutCache::layoutCreate(
          &device, &compute_readonly_descriptor_set_layout_create_info);
  compute_descriptor_set_layouts[1] =
      VulkanDescriptorSetLayoutCache::layoutCreate(
          &device, &compute_writeonly_descriptor_set_layout_create_info);

  VulkanPipeline compute_pipeline;
  compute_pipeline.createCompute(&device, compute_descriptor_set_layouts.size(),
                                 compute_descriptor_set_layouts.data(),
                                 compute_stage_create_info);

  compute_shader_module.destroy(&device);

  b8 running = true;
  while (running) {
    SDL_Event event;
    Input::begin();

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYDOWN: {
        if (!event.key.repeat) {
          Input::keyDownEvent(event);
        }
      } break;
      case SDL_KEYUP: {
        Input::keyUpEvent(event);
      } break;
      case SDL_WINDOWEVENT: {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
          running = false;
        }
      } break;
      case SDL_QUIT: {
        running = false;
      } break;
      }
    }
  }

  device.waitIdle();

  compute_pipeline.destroy(&device);
  graphics_pipeline.destroy(&device);

  VulkanDescriptorSetLayoutCache::shutdown(&device);
  VulkanDescriptorAllocator::shutdown(&device);

  for (u32 i = 0; i < swapchain.max_frames_in_flight; ++i) {
    image_available_semaphores[i].destroy(&device);
    render_finished_semaphores[i].destroy(&device);
    compute_finished_semaphores[i].destroy(&device);
    in_flight_fences[i].destroy(&device);
    compute_in_flight_fences[i].destroy(&device);
  }

  for (u32 i = 0; i < swapchain.images.size(); ++i) {
    graphics_command_buffers[i].free(&device, &graphics_command_pool);
    compute_command_buffers[i].free(&device, &compute_command_pool);
  }

  graphics_command_pool.destroy(&device);
  compute_command_pool.destroy(&device);

  allocator.destroy();

  for (u32 i = 0; i < framebuffers.size(); ++i) {
    framebuffers[i].destroy(&device);
  }

  render_pass.destroy(&device);

  swapchain.destroy(&device);

  device.destroy();

  surface.destroy(&instance);

#ifndef NDEBUG
  debug_messenger.destroy(&instance);
#endif

  instance.destroy();

  Input::shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}