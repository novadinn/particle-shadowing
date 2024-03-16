/* clang-format off */
#include "camera.h"
#include "geometry.h"
#include "core/file_system.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/platform.h"
#include "particle_system.h"
#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include "renderer/vulkan/vulkan_memory_allocator.h"
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
/* clang-format on */

struct GlobalUBO {
  glm::mat4 projection;
  glm::mat4 view;
};

struct PushConstants {
  glm::mat4 model;
  u32 shadow_index;
  f32 opacity;
};

struct PushConstantsCompute {
  glm::vec4 sun_dir;
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

  VulkanMemoryAllocator allocator;
  allocator.create(&instance, &device, application_info.apiVersion);

  VulkanSwapchain swapchain;
  swapchain.create(&device, &allocator, &surface, window_width, window_height);

  VulkanRenderPass render_pass;
  render_pass.create(&device, &swapchain);

  std::vector<VulkanFramebuffer> framebuffers;
  framebuffers.resize(swapchain.images.size());
  for (u32 i = 0; i < framebuffers.size(); ++i) {
    std::vector<VkImageView> attachments = {swapchain.image_views[i],
                                            swapchain.depth_texture.view};
    framebuffers[i].create(&device, &render_pass, attachments, window_width,
                           window_height);
  }

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
  vertex_shader_module.create(
      &device,
      FileSystem::joinPath("assets/shaders/particle.vert.spv").c_str());
  VulkanShaderModule fragment_shader_module;
  fragment_shader_module.create(
      &device,
      FileSystem::joinPath("assets/shaders/particle.frag.spv").c_str());

  const u32 sector_count = 36;
  const u32 stack_count = 18;
  std::vector<f32> sphere_vertices =
      generateSphereVertices(1, sector_count, stack_count);
  std::vector<u32> sphere_indices =
      generateSphereIndices(sector_count, stack_count);

  VulkanBuffer sphere_vertex_buffer;
  sphere_vertex_buffer.create(
      &allocator, sizeof(f32) * sphere_vertices.size(),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  sphere_vertex_buffer.loadDataStaging(&device, &allocator,
                                       sphere_vertices.data(), &graphics_queue,
                                       &graphics_command_pool);
  VulkanBuffer sphere_index_buffer;
  sphere_index_buffer.create(
      &allocator, sizeof(u32) * sphere_indices.size(),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  sphere_index_buffer.loadDataStaging(&device, &allocator,
                                      sphere_indices.data(), &graphics_queue,
                                      &graphics_command_pool);

  VkDescriptorSetLayoutBinding descriptor_set_layout_binding =
      vulkanDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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

  VkDescriptorSetLayoutBinding writeonly_descriptor_set_layout_binding =
      vulkanDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                       VK_SHADER_STAGE_FRAGMENT_BIT);
  writeonly_descriptor_set_layout_binding = vulkanDescriptorSetLayoutBinding(
      0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
  VkDescriptorSetLayoutCreateInfo
      graphics_writeonly_descriptor_set_layout_create_info = {};
  graphics_writeonly_descriptor_set_layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  graphics_writeonly_descriptor_set_layout_create_info.pNext = 0;
  graphics_writeonly_descriptor_set_layout_create_info.flags = 0;
  graphics_writeonly_descriptor_set_layout_create_info.bindingCount = 1;
  graphics_writeonly_descriptor_set_layout_create_info.pBindings =
      &writeonly_descriptor_set_layout_binding;

  std::vector<VkDescriptorSetLayout> graphics_descriptor_set_layouts;
  graphics_descriptor_set_layouts.resize(2);
  graphics_descriptor_set_layouts[0] =
      VulkanDescriptorSetLayoutCache::layoutCreate(
          &device, &graphics_descriptor_set_layout_create_info);
  graphics_descriptor_set_layouts[1] =
      VulkanDescriptorSetLayoutCache::layoutCreate(
          &device, &graphics_writeonly_descriptor_set_layout_create_info);

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
  push_constant_range.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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
      &device, &render_pass, graphics_descriptor_set_layouts.size(),
      graphics_descriptor_set_layouts.data(),
      graphics_pipeline_stage_create_infos.size(),
      graphics_pipeline_stage_create_infos.data(), 1, &push_constant_range, 0,
      0, viewport, scissor);

  vertex_shader_module.destroy(&device);
  fragment_shader_module.destroy(&device);

  VulkanBuffer global_uniform_buffer;
  global_uniform_buffer.create(&allocator, sizeof(GlobalUBO),
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               VMA_MEMORY_USAGE_CPU_TO_GPU);

  VulkanBuffer shadows_buffer;
  shadows_buffer.create(&allocator, sizeof(f32) * NUM_PARTICLES,
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        VMA_MEMORY_USAGE_GPU_ONLY);

  VulkanDescriptorSetBuilder builder;
  VkDescriptorSet global_ubo_descriptor_set;
  builder.begin();
  VkDescriptorBufferInfo global_ubo_buffer_info = {};
  global_ubo_buffer_info.buffer = global_uniform_buffer.handle;
  global_ubo_buffer_info.offset = 0;
  global_ubo_buffer_info.range = global_uniform_buffer.size;
  builder.bufferBind(0, &global_ubo_buffer_info,
                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                     VK_SHADER_STAGE_VERTEX_BIT);
  builder.end(&device, &global_ubo_descriptor_set);

  builder = {};
  VkDescriptorSet graphics_readonly_descriptor_set;
  VkDescriptorBufferInfo graphics_readonly_buffer_info = {};
  graphics_readonly_buffer_info.buffer = shadows_buffer.handle;
  graphics_readonly_buffer_info.offset = 0;
  graphics_readonly_buffer_info.range = shadows_buffer.size;
  builder.bufferBind(0, &graphics_readonly_buffer_info,
                     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     VK_SHADER_STAGE_FRAGMENT_BIT);
  builder.end(&device, &graphics_readonly_descriptor_set);

  VulkanShaderModule compute_shader_module;
  compute_shader_module.create(
      &device,
      FileSystem::joinPath("assets/shaders/particle_shadowing.comp.spv")
          .c_str());

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

  VkPushConstantRange compute_push_constant_range = {};
  compute_push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  compute_push_constant_range.offset = 0;
  compute_push_constant_range.size = sizeof(PushConstantsCompute);

  VulkanPipeline compute_pipeline;
  compute_pipeline.createCompute(&device, compute_descriptor_set_layouts.size(),
                                 compute_descriptor_set_layouts.data(), 1,
                                 &compute_push_constant_range,
                                 compute_stage_create_info);

  compute_shader_module.destroy(&device);

  VulkanBuffer compute_readonly_buffer;
  compute_readonly_buffer.create(&allocator, sizeof(Particle) * NUM_PARTICLES,
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 VMA_MEMORY_USAGE_CPU_TO_GPU);

  builder = {};
  VkDescriptorSet compute_readonly_descriptor_set;
  builder.begin();
  VkDescriptorBufferInfo compute_readonly_buffer_info = {};
  compute_readonly_buffer_info.buffer = compute_readonly_buffer.handle;
  compute_readonly_buffer_info.offset = 0;
  compute_readonly_buffer_info.range = compute_readonly_buffer.size;
  builder.bufferBind(0, &compute_readonly_buffer_info,
                     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     VK_SHADER_STAGE_COMPUTE_BIT);
  builder.end(&device, &compute_readonly_descriptor_set);

  builder = {};
  VkDescriptorSet compute_writeonly_descriptor_set;
  builder.begin();
  VkDescriptorBufferInfo compute_writeonly_buffer_info = {};
  compute_writeonly_buffer_info.buffer = shadows_buffer.handle;
  compute_writeonly_buffer_info.offset = 0;
  compute_writeonly_buffer_info.range = shadows_buffer.size;
  builder.bufferBind(0, &compute_writeonly_buffer_info,
                     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     VK_SHADER_STAGE_COMPUTE_BIT);
  builder.end(&device, &compute_writeonly_descriptor_set);

  Camera camera;
  camera.create(45, (f32)window_width / (f32)window_height, 0.1f, 1000.0f);

  ParticleSystem particle_system;
  particle_system.createExplosion(glm::vec3(0.0f));

  glm::ivec2 previous_mouse = {0, 0};
  b8 running = true;
  uint32_t current_frame = 0;
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
      case SDL_MOUSEBUTTONDOWN: {
        Input::mouseButtonDownEvent(event);
      } break;
      case SDL_MOUSEBUTTONUP: {
        Input::mouseButtonUpEvent(event);
      } break;
      case SDL_MOUSEWHEEL: {
        Input::wheelEvent(event);
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

    float delta_time = 0.01f;
    glm::ivec2 current_mouse;
    Input::getMousePosition(&current_mouse.x, &current_mouse.y);
    glm::vec2 mouse_delta = current_mouse - previous_mouse;
    mouse_delta *= delta_time;

    glm::ivec2 wheel_movement = {Input::wheel_x, Input::wheel_y};

    if (Input::wasMouseButtonHeld(SDL_BUTTON_MIDDLE)) {
      if (Input::wasKeyHeld(SDLK_LSHIFT)) {
        camera.pan(mouse_delta);
      } else {
        camera.rotate(mouse_delta);
      }
    }
    if (wheel_movement.y != 0) {
      camera.zoom(delta_time * wheel_movement.y * 5);
    }

    particle_system.update(delta_time);

    device.waitIdle();

    VulkanFence &compute_fence = compute_in_flight_fences[current_frame];
    compute_fence.wait(&device, UINT64_MAX);
    compute_fence.reset(&device);

    VulkanCommandBuffer &compute_command_buffer =
        compute_command_buffers[current_frame];
    compute_command_buffer.begin(0);

    compute_command_buffer.pipelineBind(VK_PIPELINE_BIND_POINT_COMPUTE,
                                        &compute_pipeline);
    compute_readonly_buffer.loadData(&allocator, particle_system.particles);
    compute_command_buffer.descriptorSetBind(
        &compute_pipeline, VK_PIPELINE_BIND_POINT_COMPUTE,
        compute_readonly_descriptor_set, 0, 0, 0);
    compute_command_buffer.descriptorSetBind(
        &compute_pipeline, VK_PIPELINE_BIND_POINT_COMPUTE,
        compute_writeonly_descriptor_set, 1, 0, 0);
    PushConstantsCompute push_constants_compute;
    push_constants_compute.sun_dir = glm::vec4(0.0f, 1.0f, 0.0f, 0.0);
    compute_command_buffer.pushConstants(
        &compute_pipeline, VK_SHADER_STAGE_COMPUTE_BIT, 0,
        sizeof(PushConstantsCompute), &push_constants_compute);

    compute_command_buffer.dispatch(NUM_PARTICLES / 256, 1);

    compute_command_buffer.end();

    compute_queue.submit(&compute_command_buffer, 0, 0, 1,
                         &compute_finished_semaphores[current_frame],
                         &compute_fence, 0);

    VulkanFence &graphics_fence = in_flight_fences[current_frame];
    compute_fence.wait(&device, UINT64_MAX);
    graphics_fence.wait(&device, UINT64_MAX);
    graphics_fence.reset(&device);

    VulkanSemaphore &image_available_semaphore =
        image_available_semaphores[current_frame];

    u32 image_index;
    swapchain.acquireNextImageIndex(&device, UINT64_MAX,
                                    &image_available_semaphore, &image_index);

    VulkanCommandBuffer &graphics_command_buffer =
        graphics_command_buffers[current_frame];
    graphics_command_buffer.begin(0);

    VulkanFramebuffer &framebuffer = framebuffers[image_index];

    glm::vec4 clear_color = {1, 0, 0, 1};
    glm::vec4 render_area = {0, 0, window_width, window_height};
    graphics_command_buffer.renderPassBegin(&render_pass, &framebuffer,
                                            clear_color, render_area);

    glm::vec4 viewport_values = {0.0f, render_area.w, render_area.z,
                                 -render_area.w};
    graphics_command_buffer.viewportSet(viewport_values);
    glm::vec4 scissor_values = {0, 0, render_area.z, render_area.w};
    graphics_command_buffer.scissorSet(scissor_values);

    GlobalUBO global_ubo;
    global_ubo.projection = camera.getProjectionMatrix();
    global_ubo.view = camera.getViewMatrix();
    global_uniform_buffer.loadData(&allocator, &global_ubo);

    graphics_command_buffer.pipelineBind(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                         &graphics_pipeline);
    graphics_command_buffer.bufferVertexBind(&sphere_vertex_buffer, 0);
    graphics_command_buffer.bufferIndexBind(&sphere_index_buffer, 0);
    graphics_command_buffer.descriptorSetBind(
        &graphics_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS,
        global_ubo_descriptor_set, 0, 0, 0);
    for (u32 i = 0; i < NUM_PARTICLES; ++i) {
      PushConstants push_constants;
      push_constants.model =
          glm::translate(glm::mat4(1.0f), particle_system.particles[i].pos) *
          glm::scale(glm::mat4(1.0f),
                     glm::vec3(particle_system.particles[i].radius));
      push_constants.shadow_index = i;
      push_constants.opacity = particle_system.particles[i].opacity;
      graphics_command_buffer.pushConstants(
          &graphics_pipeline,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
          sizeof(PushConstants), &push_constants);
      graphics_command_buffer.descriptorSetBind(
          &graphics_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS,
          graphics_readonly_descriptor_set, 1, 0, 0);
      graphics_command_buffer.drawIndexed(sphere_indices.size());
    }

    graphics_command_buffer.renderPassEnd();

    graphics_command_buffer.end();

    VkPipelineStageFlags wait_dst_stage_masks[2] = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::vector<VulkanSemaphore> wait_semaphores = {
        compute_finished_semaphores[current_frame],
        image_available_semaphores[current_frame]};

    graphics_queue.submit(
        &graphics_command_buffer, wait_semaphores.size(),
        wait_semaphores.data(), 1, &render_finished_semaphores[current_frame],
        &in_flight_fences[current_frame], wait_dst_stage_masks);
    graphics_queue.present(
        &swapchain, &render_finished_semaphores[current_frame], image_index);

    current_frame = (current_frame + 1) % swapchain.max_frames_in_flight;

    Input::getMousePosition(&previous_mouse.x, &previous_mouse.y);
  }

  device.waitIdle();

  shadows_buffer.destroy(&allocator);
  compute_readonly_buffer.destroy(&allocator);

  sphere_vertex_buffer.destroy(&allocator);
  sphere_index_buffer.destroy(&allocator);

  compute_pipeline.destroy(&device);

  global_uniform_buffer.destroy(&allocator);
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

  for (u32 i = 0; i < framebuffers.size(); ++i) {
    framebuffers[i].destroy(&device);
  }

  render_pass.destroy(&device);

  swapchain.destroy(&device, &allocator);

  allocator.destroy();

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