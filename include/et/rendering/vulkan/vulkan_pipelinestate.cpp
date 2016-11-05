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
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanPipelineStatePrivate
{
public:
	VulkanPipelineStatePrivate(VulkanState& v) :
		vulkan(v)
	{
	}

	VulkanState& vulkan;
	VulkanNativePipeline nativePipeline;
};

VulkanPipelineState::VulkanPipelineState(VulkanState& vulkan)
{
	ET_PIMPL_INIT(VulkanPipelineState, vulkan);
}

VulkanPipelineState::~VulkanPipelineState()
{
	vkDestroyPipeline(_private->vulkan.device, _private->nativePipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(_private->vulkan.device, _private->nativePipeline.layout, nullptr);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->nativePipeline.descriptorSetLayout, nullptr);
	ET_PIMPL_FINALIZE(VulkanPipelineState);
}

const VulkanNativePipeline& VulkanPipelineState::nativePipeline() const
{
	return _private->nativePipeline;
}

void VulkanPipelineState::build()
{
	VulkanRenderPass::Pointer pass = renderPass();

	VkPipelineColorBlendStateCreateInfo blendInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	{
		VkPipelineColorBlendAttachmentState attachmentInfo = { };
		attachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
		rasterizerInfo.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
		rasterizerInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
		rasterizerInfo.lineWidth = 1.0f;
	}

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	{
		assemblyInfo.primitiveRestartEnable = VK_FALSE;
		assemblyInfo.topology = vulkan::primitiveTopology(primitiveType());
	}

	VkPipelineVertexInputStateCreateInfo vertexInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	{
		Vector<VkVertexInputAttributeDescription> attribs;
		attribs.reserve(inputLayout().numElements());
		for (const auto& e : inputLayout().elements())
		{
			attribs.emplace_back();
			attribs.back().offset = e.offset();
			attribs.back().format = vulkan::dataTypeValue(e.type());
			attribs.back().location = static_cast<uint32_t>(e.usage());
		}

		VkVertexInputBindingDescription binding = { };
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		binding.stride = inputLayout().totalSize();

		vertexInfo.pVertexBindingDescriptions = &binding;
		vertexInfo.vertexBindingDescriptionCount = 1;
		vertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
		vertexInfo.pVertexAttributeDescriptions = attribs.data();
	}

	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	{
		VkDynamicState dynamicStates = VK_DYNAMIC_STATE_VIEWPORT;
		dynamicState.dynamicStateCount = 1;
		dynamicState.pDynamicStates = &dynamicStates;
	}

	VkPipelineViewportStateCreateInfo viewportInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	{
		viewportInfo.pViewports = &pass->nativeRenderPass().viewport;
		viewportInfo.viewportCount = 1;
		viewportInfo.pScissors = &pass->nativeRenderPass().scissor;
		viewportInfo.scissorCount = 1;
	}
	
	VkDescriptorSetLayoutBinding binding = { };
	binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	binding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	
	VkDescriptorSetLayoutCreateInfo descriptorSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetInfo.bindingCount = 1;
	descriptorSetInfo.pBindings = &binding;
	VULKAN_CALL(vkCreateDescriptorSetLayout(_private->vulkan.device, &descriptorSetInfo, nullptr, &_private->nativePipeline.descriptorSetLayout));

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutInfo.pSetLayouts = &_private->nativePipeline.descriptorSetLayout;
	layoutInfo.setLayoutCount = 1;
	VULKAN_CALL(vkCreatePipelineLayout(_private->vulkan.device, &layoutInfo, nullptr, &_private->nativePipeline.layout));

	VkGraphicsPipelineCreateInfo info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	{
		info.pColorBlendState = &blendInfo;
		info.pDepthStencilState = &depthInfo;
		info.pInputAssemblyState = &assemblyInfo;
		info.pMultisampleState = &msInfo;
		info.pRasterizationState = &rasterizerInfo;
		info.pVertexInputState = &vertexInfo;
		info.pDynamicState = &dynamicState;
		info.pViewportState = &viewportInfo;
		info.layout = _private->nativePipeline.layout;
		info.renderPass = pass->nativeRenderPass().renderPass;
		info.pStages = VulkanProgram::Pointer(program())->shaderModules().stageCreateInfo;
		info.stageCount = 2;
	}
	VULKAN_CALL(vkCreateGraphicsPipelines(_private->vulkan.device, _private->vulkan.pipelineCache, 1, &info, nullptr, &_private->nativePipeline.pipeline));
}

}
