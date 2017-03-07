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

	void generatePipelineLayout(const Program::Reflection& reflection, const VulkanRenderPass::Pointer& pass);

	VulkanRenderer* renderer = nullptr;
	VulkanState& vulkan;
};

VulkanPipelineState::VulkanPipelineState(VulkanRenderer* renderer, VulkanState& vulkan)
{
	ET_PIMPL_INIT(VulkanPipelineState, renderer, vulkan);
}

VulkanPipelineState::~VulkanPipelineState()
{
	if (_private->pipeline)
		vkDestroyPipeline(_private->vulkan.device, _private->pipeline, nullptr);

	if (_private->layout)
		vkDestroyPipelineLayout(_private->vulkan.device, _private->layout, nullptr);

	if (_private->texturesLayout)
		vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->texturesLayout, nullptr);

	ET_PIMPL_FINALIZE(VulkanPipelineState);
}

const VulkanNativePipeline& VulkanPipelineState::nativePipeline() const
{
	return (*_private);
}

void VulkanPipelineState::build(const RenderPass::Pointer& inPass)
{
	VulkanProgram::Pointer prog = program();
	if ((prog->shaderModules().vertex == nullptr) || (prog->shaderModules().fragment == nullptr))
		return;

	VulkanRenderPass::Pointer pass = inPass;
	_renderPassId = pass->identifier();
	
	Vector<VkPipelineColorBlendAttachmentState> attachmentInfo;
	attachmentInfo.reserve(MaxRenderTargets);

	for (const RenderTarget& rt : pass->info().color)
	{
		if (rt.enabled)
		{
			attachmentInfo.emplace_back();
			VkPipelineColorBlendAttachmentState& state = attachmentInfo.back();
			state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			state.blendEnable = blendState().enabled ? VK_TRUE : VK_FALSE;
			state.colorBlendOp = vulkan::blendOperationValue(blendState().colorOperation);
			state.srcColorBlendFactor = vulkan::blendFactorValur(blendState().color.source);
			state.dstColorBlendFactor = vulkan::blendFactorValur(blendState().color.dest);
			state.alphaBlendOp = vulkan::blendOperationValue(blendState().alphaOperation);
			state.srcAlphaBlendFactor = vulkan::blendFactorValur(blendState().alpha.source);
			state.dstAlphaBlendFactor = vulkan::blendFactorValur(blendState().alpha.dest);
		}
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
		rasterizerInfo.lineWidth = 1.0f;
		rasterizerInfo.depthBiasConstantFactor = pass->info().depthBias;
		rasterizerInfo.depthBiasSlopeFactor = pass->info().depthSlope;
		rasterizerInfo.depthBiasEnable = (rasterizerInfo.depthBiasConstantFactor > 0.0f) || (rasterizerInfo.depthBiasSlopeFactor > 0.0f);
	}

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	{
		assemblyInfo.topology = vulkan::primitiveTopology(primitiveType());
	}
	
	_private->generatePipelineLayout(prog->reflection(), pass);

	VkVertexInputBindingDescription binding = { };
	Vector<VkVertexInputAttributeDescription> attribs;
	VkPipelineVertexInputStateCreateInfo vertexInfo = { };
	_private->generateInputLayout(inputLayout(), prog->reflection().inputLayout, vertexInfo, attribs, binding);

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO }; 
	viewportState.scissorCount = 1;
	viewportState.viewportCount = 1;

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

void VulkanPipelineStatePrivate::generatePipelineLayout(const Program::Reflection& reflection, const VulkanRenderPass::Pointer& pass)
{
	Set<uint32_t> textureBindings;
	for (auto& tex : reflection.textures.vertexTextures)
		textureBindings.insert(tex.second);
	for (auto& tex : reflection.textures.fragmentTextures)
		textureBindings.insert(tex.second);
	
	Set<uint32_t> samplerBindings;
	for (auto& tex : reflection.textures.vertexSamplers)
		samplerBindings.insert(tex.second);
	for (auto& tex : reflection.textures.fragmentSamplers)
		samplerBindings.insert(tex.second);
	
	{
		Vector<VkDescriptorSetLayoutBinding> textureLayoutBindings;
		textureLayoutBindings.reserve(MaterialTexturesCount);
		for (uint32_t textureBinding : textureBindings)
		{
			textureLayoutBindings.emplace_back();
			textureLayoutBindings.back().binding = textureBinding;
			textureLayoutBindings.back().stageFlags = VK_SHADER_STAGE_ALL;
			textureLayoutBindings.back().descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			textureLayoutBindings.back().descriptorCount = 1;
		}
		for (uint32_t samplerBinding : samplerBindings)
		{
			textureLayoutBindings.emplace_back();
			textureLayoutBindings.back().binding = samplerBinding;
			textureLayoutBindings.back().stageFlags = VK_SHADER_STAGE_ALL;
			textureLayoutBindings.back().descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			textureLayoutBindings.back().descriptorCount = 1;
		}		

		VkDescriptorSetLayoutCreateInfo layoutSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutSetInfo.bindingCount = static_cast<uint32_t>(textureLayoutBindings.size());
		layoutSetInfo.pBindings = textureLayoutBindings.data();
		VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &layoutSetInfo, nullptr, &texturesLayout));
	}

	VkDescriptorSetLayout layouts[DescriptorSetClass::Count] =
	{
		pass->nativeRenderPass().dynamicDescriptorSetLayout,
		texturesLayout,
	};

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutInfo.pSetLayouts = layouts;
	layoutInfo.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
	VULKAN_CALL(vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &layout));
}

void VulkanPipelineStatePrivate::generateInputLayout(const VertexDeclaration& inputLayout, const VertexDeclaration& expectedLayout,
	VkPipelineVertexInputStateCreateInfo& vertexInfo, Vector<VkVertexInputAttributeDescription>& attribs,
	VkVertexInputBindingDescription& binding)
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
		else
		{
			log::warning("Vertex attrib %s ignored (not present in required layout)",
				vertexAttributeUsageToString(e.usage()).c_str());
		}
	}

	binding = { };
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding.stride = inputLayout.sizeInBytes();

	vertexInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInfo.pVertexBindingDescriptions = &binding;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
	vertexInfo.pVertexAttributeDescriptions = attribs.data();
}

}
