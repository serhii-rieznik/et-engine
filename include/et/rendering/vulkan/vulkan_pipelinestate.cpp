/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_pipelinestate.h>
#include <et/rendering/vulkan/vulkan_vertexbuffer.h>
#include <et/rendering/vulkan/vulkan_indexbuffer.h>
#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan_textureset.h>
#include <et/rendering/vulkan/vulkan_sampler.h>
#include <et/rendering/vulkan/vulkan_renderer.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{
class VulkanPipelineStatePrivate : public VulkanNativePipeline
{
public:
	VulkanPipelineStatePrivate(VulkanRenderer* r, VulkanState& v) :
		renderer(r), vulkan(v)
	{
	}

	void generateInputLayout(const VertexDeclaration& decl, 
		VkPipelineVertexInputStateCreateInfo& vertexInfo, Vector<VkVertexInputAttributeDescription>& attribs,
		VkVertexInputBindingDescription& binding);

	void generatePipelineLayout(const Program::Reflection& reflection, VulkanRenderPass::Pointer pass);

	VulkanRenderer* renderer = nullptr;
	VulkanState& vulkan;
};

VulkanPipelineState::VulkanPipelineState(VulkanRenderer* renderer, VulkanState& vulkan)
{
	ET_PIMPL_INIT(VulkanPipelineState, renderer, vulkan);
}

VulkanPipelineState::~VulkanPipelineState()
{
	vkDestroyPipeline(_private->vulkan.device, _private->pipeline, nullptr);
	vkDestroyPipelineLayout(_private->vulkan.device, _private->layout, nullptr);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->texturesLayout, nullptr);
	ET_PIMPL_FINALIZE(VulkanPipelineState);
}

const VulkanNativePipeline& VulkanPipelineState::nativePipeline() const
{
	return (*_private);
}

void VulkanPipelineState::build()
{
	VkPipelineColorBlendAttachmentState attachmentInfo = { };
	VkPipelineColorBlendStateCreateInfo blendInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	{
		attachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendInfo.pAttachments = &attachmentInfo;
		blendInfo.attachmentCount = 1;
	}

	VkPipelineDepthStencilStateCreateInfo depthInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	{
		depthInfo.depthTestEnable = VK_TRUE;
		depthInfo.depthWriteEnable = depthState().depthWriteEnabled ? VK_TRUE : VK_FALSE;
		depthInfo.depthCompareOp = vulkan::depthCompareOperation(depthState().compareFunction);
	}
		
	VkPipelineMultisampleStateCreateInfo msInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	{
		msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	}
	
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	{
		rasterizerInfo.cullMode = vulkan::cullModeFlags(cullMode());
		rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
		rasterizerInfo.lineWidth = 1.0f;
	}

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	{
		assemblyInfo.topology = vulkan::primitiveTopology(primitiveType());
	}
	
	VulkanProgram::Pointer prog = program();
	VulkanRenderPass::Pointer pass = renderPass();
	_private->generatePipelineLayout(prog->reflection(), pass);

	VkVertexInputBindingDescription binding = { };
	Vector<VkVertexInputAttributeDescription> attribs;
	VkPipelineVertexInputStateCreateInfo vertexInfo = { };
	_private->generateInputLayout(inputLayout(), vertexInfo, attribs, binding);

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO }; 
	viewportState.scissorCount = 1;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &pass->nativeRenderPass().viewport;
	viewportState.pScissors = &pass->nativeRenderPass().scissor;

	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	{
		dynamicState.pDynamicStates = dynamicStates;
		dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
	}

	VkGraphicsPipelineCreateInfo info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	{
		info.pColorBlendState = &blendInfo;
		info.pDepthStencilState = &depthInfo;
		info.pInputAssemblyState = &assemblyInfo;
		info.pMultisampleState = &msInfo;
		info.pRasterizationState = &rasterizerInfo;
		info.pVertexInputState = &vertexInfo;
		info.pDynamicState = &dynamicState;
		info.pViewportState = &viewportState;
		info.layout = _private->layout;
		info.pStages = prog->shaderModules().stageCreateInfo;
		info.renderPass = pass->nativeRenderPass().renderPass;
		info.stageCount = 2;
	}
	VULKAN_CALL(vkCreateGraphicsPipelines(_private->vulkan.device, _private->vulkan.pipelineCache, 1, &info, nullptr, &_private->pipeline));
}

void VulkanPipelineState::bind(VulkanRenderPass::Pointer pass, MaterialInstance::Pointer& material)
{

}

void VulkanPipelineStatePrivate::generatePipelineLayout(const Program::Reflection& reflection, VulkanRenderPass::Pointer pass)
{
	Set<uint32_t> allTextureBindings;
	for (auto& tex : reflection.textures.vertexTextures)
		allTextureBindings.insert(tex.second);
	for (auto& tex : reflection.textures.fragmentTextures)
		allTextureBindings.insert(tex.second);

	Vector<VkDescriptorSetLayoutBinding> textureBindings;
	textureBindings.reserve(MaterialTexturesCount);
	for (uint32_t textureBinding : allTextureBindings)
	{
		textureBindings.emplace_back();
		textureBindings.back().binding = textureBinding;
		textureBindings.back().stageFlags = VK_SHADER_STAGE_ALL;
		textureBindings.back().descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureBindings.back().descriptorCount = 1;
	}

	VkDescriptorSetLayoutCreateInfo layoutSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutSetInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
	layoutSetInfo.pBindings = textureBindings.data();
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &layoutSetInfo, nullptr, &texturesLayout));

	VkDescriptorSetLayout layouts[] = 
	{
		pass->nativeRenderPass().dynamicDescriptorSetLayout,
		texturesLayout
	};

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutInfo.pSetLayouts = layouts;
	layoutInfo.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
	VULKAN_CALL(vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &layout));
}

void VulkanPipelineStatePrivate::generateInputLayout(const VertexDeclaration& decl, 
	VkPipelineVertexInputStateCreateInfo& vertexInfo, Vector<VkVertexInputAttributeDescription>& attribs,
	VkVertexInputBindingDescription& binding)
{
	attribs.clear();
	attribs.reserve(decl.numElements());
	for (const auto& e : decl.elements())
	{
		attribs.emplace_back();
		attribs.back().offset = e.offset();
		attribs.back().format = vulkan::dataTypeValue(e.type());
		attribs.back().location = static_cast<uint32_t>(e.usage());
	}

	binding = { };
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding.stride = decl.totalSize();

	vertexInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInfo.pVertexBindingDescriptions = &binding;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
	vertexInfo.pVertexAttributeDescriptions = attribs.data();
}

}
