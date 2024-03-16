cmake_minimum_required(VERSION 3.9)

project(particle-shadowing)

set(CMAKE_CXX_STANDARD 23)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/SDL2/cmake/")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/")

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

if (DEFINED VULKAN_SDK_PATH)
  set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
  set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
  set(Vulkan_FOUND "True")
else()
  find_package(Vulkan REQUIRED)
  message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()

add_definitions(
  -DGLM_FORCE_LEFT_HANDED
  -DGLM_ENABLE_EXPERIMENTAL
  -DSTB_IMAGE_IMPLEMENTATION
)

include_directories(
  ${Vulkan_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanMemoryAllocator/include
)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/core/logger.cpp
  src/core/input.cpp
  src/renderer/vulkan/vulkan_instance.cpp
  src/renderer/vulkan/vulkan_debug_messenger.cpp
  src/renderer/vulkan/vulkan_surface.cpp
  src/renderer/vulkan/vulkan_device.cpp
  src/renderer/vulkan/vulkan_swapchain.cpp
  src/renderer/vulkan/vulkan_render_pass.cpp
  src/renderer/vulkan/vulkan_framebuffer.cpp
  src/renderer/vulkan/vulkan_memory_allocator.cpp
  src/renderer/vulkan/vulkan_queue.cpp
  src/renderer/vulkan/vulkan_command_pool.cpp
  src/renderer/vulkan/vulkan_command_buffer.cpp
  src/renderer/vulkan/vulkan_semaphore.cpp
  src/renderer/vulkan/vulkan_fence.cpp
  src/renderer/vulkan/vulkan_descriptor_allocator.cpp
  src/renderer/vulkan/vulkan_shader_module.cpp
  src/renderer/vulkan/vulkan_pipeline.cpp
  src/renderer/vulkan/vulkan_descriptor_set_layout_cache.cpp
  src/renderer/vulkan/vulkan_descriptor_set_builder.cpp
  src/renderer/vulkan/vulkan_buffer.cpp
  src/renderer/vulkan/vulkan_texture.cpp
  src/renderer/vulkan/vulkan_texture.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
  ${SDL2_LIBRARIES}
  ${Vulkan_LIBRARIES}
)

file(GLOB_RECURSE VK_GLSL_SOURCE_FILES
  "assets/shaders/*.vert"
  "assets/shaders/*.tesc"
  "assets/shaders/*.tese"
  "assets/shaders/*.geom"
  "assets/shaders/*.frag"
  "assets/shaders/*.comp"
)
set(GLSLANG "glslangValidator")
foreach(GLSL ${VK_GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/assets/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/assets/shaders/"
    COMMAND ${GLSLANG} --target-env vulkan1.2 ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
  shaders 
  DEPENDS ${SPIRV_BINARY_FILES}
)
add_dependencies(${PROJECT_NAME} shaders)