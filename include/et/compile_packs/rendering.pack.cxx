#include <et/core/et.h>

#include "../rendering/rendering.cpp"
#include "../rendering/shadersource.cpp"
#include "../rendering/sharedconstbuffer.cpp"

#include "../rendering/base/helpers.cpp"
#include "../rendering/base/indexarray.cpp"
#include "../rendering/base/material.cpp"
#include "../rendering/base/materiallibrary.cpp"
#include "../rendering/base/pipelinestate.cpp"
#include "../rendering/base/primitives.cpp"
#include "../rendering/base/renderbatch.cpp"
#include "../rendering/base/vertexarray.cpp"
#include "../rendering/base/vertexdatachunk.cpp"
#include "../rendering/base/vertexdeclaration.cpp"
#include "../rendering/base/vertexstorage.cpp"

#if (ET_PLATFORM_WIN)
#	include "../rendering/dx12/dx12_indexbuffer.cpp"
#	include "../rendering/dx12/dx12_pipelinestate.cpp"
#	include "../rendering/dx12/dx12_program.cpp"
#	include "../rendering/dx12/dx12_renderer.cpp"
#	include "../rendering/dx12/dx12_renderpass.cpp"
#	include "../rendering/dx12/dx12_texture.cpp"
#	include "../rendering/dx12/dx12_vertexbuffer.cpp"

#	include "../rendering/vulkan/vulkan.cpp"
#	include "../rendering/vulkan/vulkan_databuffer.cpp"
#	include "../rendering/vulkan/vulkan_indexbuffer.cpp"
#	include "../rendering/vulkan/vulkan_pipelinestate.cpp"
#	include "../rendering/vulkan/vulkan_program.cpp"
#	include "../rendering/vulkan/vulkan_renderer.cpp"
#	include "../rendering/vulkan/vulkan_renderpass.cpp"
#	include "../rendering/vulkan/vulkan_texture.cpp"
#	include "../rendering/vulkan/vulkan_sampler.cpp"
#	include "../rendering/vulkan/vulkan_vertexbuffer.cpp"
#	include "../rendering/vulkan/glslang/vulkan_glslang.cpp"
#endif
