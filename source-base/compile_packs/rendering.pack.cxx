#include <et/core/et.h>

#include "../rendering/material.cpp"
#include "../rendering/rendering.cpp"
#include "../rendering/vertexbufferfactory.cpp"

#include "../rendering/base/helpers.cpp"
#include "../rendering/base/indexarray.cpp"
#include "../rendering/base/pipelinestate.cpp"
#include "../rendering/base/primitives.cpp"
#include "../rendering/base/renderbatch.cpp"
#include "../rendering/base/vertexarray.cpp"
#include "../rendering/base/vertexdatachunk.cpp"
#include "../rendering/base/vertexdeclaration.cpp"
#include "../rendering/base/vertexstorage.cpp"

#if (ET_PLATFORM_WIN)
#	include "dx12/dx12_indexbuffer.cpp"
#	include "dx12/dx12_pipelinestate.cpp"
#	include "dx12/dx12_program.cpp"
#	include "dx12/dx12_renderer.cpp"
#	include "dx12/dx12_renderpass.cpp"
#	include "dx12/dx12_texture.cpp"
#	include "dx12/dx12_vertexbuffer.cpp"
#	include "vulkan/vulkan.cpp"
#	include "vulkan/vulkan_indexbuffer.cpp"
#	include "vulkan/vulkan_pipelinestate.cpp"
#	include "vulkan/vulkan_program.cpp"
#	include "vulkan/vulkan_renderer.cpp"
#	include "vulkan/vulkan_renderpass.cpp"
#	include "vulkan/vulkan_texture.cpp"
#	include "vulkan/vulkan_vertexbuffer.cpp"
#endif

#include "../rendering/opengl/opengl.cpp"
#include "../rendering/opengl/opengl_caps.cpp"
#include "../rendering/opengl/opengl_framebuffer.cpp"
#include "../rendering/opengl/opengl_indexbuffer.cpp"
#include "../rendering/opengl/opengl_materialfactory.cpp"
#include "../rendering/opengl/opengl_pipelinestate.cpp"
#include "../rendering/opengl/opengl_program.cpp"
#include "../rendering/opengl/opengl_renderer.cpp"
#include "../rendering/opengl/opengl_renderpass.cpp"
#include "../rendering/opengl/opengl_texture.cpp"
#include "../rendering/opengl/opengl_vertexarrayobject.cpp"
#include "../rendering/opengl/opengl_vertexbuffer.cpp"
