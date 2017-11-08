/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_buffer.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan_sampler.h>
#include <et/rendering/vulkan/vulkan_textureset.h>
#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan_pipelinestate.h>
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

	void generateInputLayout(const VertexDeclaration& inputLayout, const VertexDeclaration& expectedLayout,
		VkPipelineVertexInputStateCreateInfo& vertexInfo, Vector<VkVertexInputAttributeDescription>& attribs,
		VkVertexInputBindingDescription& binding);

	VulkanRenderer* renderer = nullptr;
	VulkanState& vulkan;
};

VulkanPipelineState::VulkanPipelineState(VulkanRenderer* renderer, VulkanState& vulkan)
{
	ET_PIMPL_INIT(VulkanPipelineState, renderer, vulkan);
}

VulkanPipelineState::~VulkanPipelineState()
{
	_private->cleanup(_private->vulkan);
	ET_PIMPL_FINALIZE(VulkanPipelineState);
}

const VulkanNativePipeline& VulkanPipelineState::nativePipeline() const
{
	return (*_private);
}

void VulkanPipelineState::build(const RenderPass::Pointer& inPass)
{
	VulkanRenderPass::Pointer pass = inPass;
	_renderPassId = pass->identifier();
	
	Vector<VkPipelineColorBlendAttachmentState> attachmentInfo;
	attachmentInfo.reserve(MaxRenderTargets);

	for (const RenderTarget& rt : pass->info().color)
	{
		if (rt.targetClass == RenderTarget::Class::Disabled)
			break;

		attachmentInfo.emplace_back();
		VkPipelineColorBlendAttachmentState& state = attachmentInfo.back();
		state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		state.blendEnable = blendState().enabled ? VK_TRUE : VK_FALSE;
		state.colorBlendOp = vulkan::blendOperationValue(blendState().colorOperation);
		state.srcColorBlendFactor = vulkan::blendFactorValue(blendState().color.source);
		state.dstColorBlendFactor = vulkan::blendFactorValue(blendState().color.dest);
		state.alphaBlendOp = vulkan::blendOperationValue(blendState().alphaOperation);
		state.srcAlphaBlendFactor = vulkan::blendFactorValue(blendState().alpha.source);
		state.dstAlphaBlendFactor = vulkan::blendFactorValue(blendState().alpha.dest);
	}
	VkPipelineColorBlendStateCreateInfo blendInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	{
		blendInfo.pAttachments = attachmentInfo.data();
		blendInfo.attachmentCount = static_cast<uint32_t>(attachmentInfo.size());
	}

	VkPipelineDepthStencilStateCreateInfo depthInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	{
		depthInfo.depthTestEnable = depthState().depthTestEnabled ? VK_TRUE : VK_FALSE;
		depthInfo.depthWriteEnable = depthState().depthWriteEnabled ? VK_TRUE : VK_FALSE;
		depthInfo.depthCompareOp = vulkan::depthCompareOperation(depthState().compareFunction);
	}
		
	VkPipelineMultisampleStateCreateInfo msInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	{
		msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		msInfo.alphaToCoverageEnable = blendState().alphaToCoverageEnabled ? VK_TRUE : VK_FALSE;
	}
	
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	{
		rasterizerInfo.cullMode = vulkan::cullModeFlags(cullMode());
		rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
		rasterizerInfo.depthBiasEnable = pass->info().enableDepthBias;
		rasterizerInfo.lineWidth = 1.0f;
	}

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	{
		assemblyInfo.topology = vulkan::primitiveTopology(primitiveType());
	}
	
	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.scissorCount = 1;
	viewportState.viewportCount = 1;

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	{
		dynamicState.pDynamicStates = dynamicStates;
		dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
	}

	VkVertexInputBindingDescription binding = {};
	Vector<VkVertexInputAttributeDescription> attribs;
	VkPipelineVertexInputStateCreateInfo vertexInfo = {};
	Vector<VkPipelineShaderStageCreateInfo> stages;

	VulkanProgram::Pointer prog = program();
	if (prog.valid())
	{
		_private->buildLayout(_private->vulkan, prog->reflection(), pass->nativeRenderPass().dynamicDescriptorSetLayout);
		_private->generateInputLayout(inputLayout(), prog->reflection().inputLayout, vertexInfo, attribs, binding);
		stages.reserve(prog->shaderModules().stageCreateInfos.size());
		for (const auto& stage : prog->shaderModules().stageCreateInfos)
			stages.emplace_back(stage.second);
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
		info.pStages = stages.data();
		info.renderPass = pass->nativeRenderPass().renderPass;
		info.stageCount = static_cast<uint32_t>(stages.size());
	}
	
	if (prog.valid() && (info.stageCount == 2))
	{
		VULKAN_CALL(vkCreateGraphicsPipelines(_private->vulkan.device, _private->vulkan.pipelineCache, 1, &info, nullptr, &_private->pipeline));
	}
}

void VulkanPipelineStatePrivate::generateInputLayout(const VertexDeclaration& inputLayout, const VertexDeclaration& expectedLayout,
	VkPipelineVertexInputStateCreateInfo& vertexInfo, Vector<VkVertexInputAttributeDescription>& attribs,
	VkVertexInputBindingDescription& binding)
{
	binding = {};
	vertexInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	if (expectedLayout.elements().empty())
	{
		printf(".");
	}
	else
	{
		attribs.clear();
		attribs.reserve(inputLayout.numElements());

		for (const auto& e : inputLayout.elements())
		{
			if (expectedLayout.has(e.usage()))
			{
				attribs.emplace_back();
				attribs.back().offset = e.offset();
				attribs.back().format = vulkan::dataTypeValue(e.type());
				attribs.back().location = static_cast<uint32_t>(e.usage());
			}
		}

		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		binding.stride = inputLayout.sizeInBytes();

		vertexInfo.pVertexBindingDescriptions = &binding;
		vertexInfo.vertexBindingDescriptionCount = 1;
		vertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
		vertexInfo.pVertexAttributeDescriptions = attribs.data();
	}
}

}
