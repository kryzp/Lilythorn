﻿cmake_minimum_required (VERSION 3.8)
project(lilythorn)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/common.cpp
    src/app.cpp
    src/platform.cpp
    src/renderer.cpp
    src/camera.cpp
    src/entity.cpp
    src/gpu_particles.cpp

    src/graphics/backbuffer.cpp
    src/graphics/backend.cpp
    src/graphics/colour.cpp
    src/graphics/compute_pipeline.cpp
    src/graphics/descriptor_builder.cpp
    src/graphics/descriptor_cache.cpp
    src/graphics/descriptor_pool_mgr.cpp
    src/graphics/gpu_buffer.cpp
    src/graphics/gpu_buffer_mgr.cpp
    src/graphics/graphics_pipeline.cpp
    src/graphics/generic_render_target.cpp
    src/graphics/render_info_builder.cpp
    src/graphics/render_target.cpp
    src/graphics/render_target_mgr.cpp
    src/graphics/shader.cpp
    src/graphics/shader_mgr.cpp
    src/graphics/shader_buffer.cpp
    src/graphics/shader_buffer_mgr.cpp
    src/graphics/sub_mesh.cpp
    src/graphics/pipeline.cpp
    src/graphics/mesh.cpp
    src/graphics/material.cpp
    src/graphics/texture.cpp
    src/graphics/texture_mgr.cpp
    src/graphics/texture_sampler.cpp
    src/graphics/util.cpp
    src/graphics/image.cpp
    src/graphics/queue.cpp
    src/graphics/vertex_descriptor.cpp
    
    src/math/timer.cpp

    src/input/input.cpp
    src/input/v_key.cpp

    src/io/endian.cpp
    src/io/stream.cpp
    src/io/file_stream.cpp
    src/io/memory_stream.cpp

    src/third_party/vk_mem_alloc.cpp
)

set(DEBUG_MODE true CACHE BOOL "Enable debug mode")
set(MAC_SUPPORT true CACHE BOOL "Enable mac support")

if(DEBUG_MODE)
	add_compile_definitions(LLT_DEBUG)
endif()

if(MAC_SUPPORT)
	add_compile_definitions(LLT_MAC_SUPPORT)

	find_package(SDL3 REQUIRED)
	add_compile_definitions(RVN_USE_SDL3)
	target_link_libraries(${PROJECT_NAME} SDL3::SDL3)

	find_package(Vulkan REQUIRED)
	add_compile_definitions(RVN_USE_VULKAN)
	target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

	find_package(glm REQUIRED)
	target_link_libraries(${PROJECT_NAME} glm::glm)
else()
	set(GLM_INCLUDE_DIRS "D:/DevLibs/glm/")

	set(SDL3_INCLUDE_DIRS "D:/DevLibs/SDL3/include/")
	set(SDL3_LIBRARIES "D:/DevLibs/SDL3/VisualC/x64/Release/SDL3.lib")

	set(VK_INCLUDE_DIRS "D:/VulkanSDK/1.3.296.0/Include/")
	set(VK_LIBRARIES "D:/VulkanSDK/1.3.296.0/Lib/vulkan-1.lib")

	target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL3_LIBRARIES} ${VK_LIBRARIES})
	target_include_directories(${PROJECT_NAME} PUBLIC ${SDL3_INCLUDE_DIRS} ${VK_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})
endif()
