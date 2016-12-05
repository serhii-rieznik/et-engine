#include <et/core/et.h>

#if (ET_PLATFORM_WIN)
#	include "../rendering/dx12/dx12.cpp"
#	include "../rendering/dx12/dx12_pipelinestate.cpp"
#	include "../rendering/dx12/dx12_program.cpp"
#	include "../rendering/dx12/dx12_renderer.cpp"
#	include "../rendering/dx12/dx12_renderpass.cpp"
#	include "../rendering/dx12/dx12_texture.cpp"

#	include "../rendering/vulkan/vulkan.cpp"
#	include "../rendering/vulkan/vulkan_buffer.cpp"
#	include "../rendering/vulkan/vulkan_pipelinestate.cpp"
#	include "../rendering/vulkan/vulkan_program.cpp"
#	include "../rendering/vulkan/vulkan_renderer.cpp"
#	include "../rendering/vulkan/vulkan_renderpass.cpp"
#	include "../rendering/vulkan/vulkan_texture.cpp"
#	include "../rendering/vulkan/vulkan_textureset.cpp"
#	include "../rendering/vulkan/vulkan_sampler.cpp"
#	include "../rendering/vulkan/glslang/vulkan_glslang.cpp"
#endif
