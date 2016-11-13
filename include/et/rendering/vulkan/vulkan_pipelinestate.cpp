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

	void generatePipelineLayout(const PipelineState::Reflection& reflection, VulkanRenderPass::Pointer pass);

	VulkanRenderer* renderer = nullptr;
	VulkanState& vulkan;

	Vector<VkWriteDescriptorSet> writeDescriptorSets;
	Vector<VkDescriptorImageInfo> textureInfoPool;

	uint32_t objectVariablesIndex = InvalidIndex;
	uint32_t materialVariablesIndex = InvalidIndex;
	uint32_t passVariablesIndex = InvalidIndex;
};

VulkanPipelineState::VulkanPipelineState(VulkanRenderer* renderer, VulkanState& vulkan)
{
	ET_PIMPL_INIT(VulkanPipelineState, renderer, vulkan);
}

VulkanPipelineState::~VulkanPipelineState()
{
	vkDestroyPipeline(_private->vulkan.device, _private->pipeline, nullptr);
	vkDestroyPipelineLayout(_private->vulkan.device, _private->layout, nullptr);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->texturesSetLayout, nullptr);
	vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptprPool, 1, &_private->texturesSet);
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
	reflection = prog->reflection();
	buildBuffers();

	printReflection();
	
	VulkanRenderPass::Pointer pass = renderPass();
	_private->generatePipelineLayout(reflection, pass);

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
	vkCmdBindPipeline(pass->nativeRenderPass().commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _private->pipeline);

	uint32_t objectBufferOffset = 0;
	if (reflection.objectVariablesBufferSize > 0)
	{
		uint8_t* dst = pass->dynamicConstantBuffer().dynamicAllocate(reflection.objectVariablesBufferSize, objectBufferOffset);
		memcpy(dst, objectVariablesBuffer.data(), reflection.objectVariablesBufferSize);
	}

	VkDescriptorSet descriptors[DescriptorSetClass::Count] = 
	{
		pass->nativeRenderPass().dynamicDescriptorSet,
		_private->texturesSet
	};
	uint32_t dynamicOffsets[] = { objectBufferOffset, 0 };

	vkCmdBindDescriptorSets(pass->nativeRenderPass().commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		_private->layout, 0, 2, descriptors, 2, dynamicOffsets);
}

void VulkanPipelineStatePrivate::generatePipelineLayout(const PipelineState::Reflection& reflection, VulkanRenderPass::Pointer pass)
{
	Vector<VkDescriptorSetLayoutBinding> textureBindings;

	textureBindings.reserve(8);
	textureInfoPool.resize(8);

	auto addUniform = [&](uint32_t bindingIndex, VkDescriptorType descType)
	{
		Vector<VkDescriptorSetLayoutBinding>* bindings = nullptr;
		switch (descType)
		{
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			bindings = &textureBindings;
			break;
		default:
			ET_FAIL("Invalid or unsupported binding type provided");
			break;
		}

		bindings->emplace_back();
		bindings->back().binding = bindingIndex;
		bindings->back().stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings->back().descriptorType = descType;
		bindings->back().descriptorCount = 1;

		if (descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			writeDescriptorSets.emplace_back();
			writeDescriptorSets.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets.back().descriptorCount = bindings->back().descriptorCount;
			writeDescriptorSets.back().descriptorType = bindings->back().descriptorType;
			writeDescriptorSets.back().dstBinding = bindings->back().binding;
		}
	};

	for (const auto& tex : reflection.fragmentTextures)
		addUniform(tex.second, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	VkDescriptorSetLayoutCreateInfo layoutSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutSetInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
	layoutSetInfo.pBindings = textureBindings.data();
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &layoutSetInfo, nullptr, &texturesSetLayout));

	VkDescriptorSetLayout layouts[] = 
	{
		pass->nativeRenderPass().dynamicDescriptorSetLayout,
		texturesSetLayout
	};

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutInfo.pSetLayouts = layouts;
	layoutInfo.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
	VULKAN_CALL(vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &layout));

	VkDescriptorSetAllocateInfo setInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	setInfo.descriptorPool = vulkan.descriptprPool;
	setInfo.pSetLayouts = &texturesSetLayout;
	setInfo.descriptorSetCount = 1;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &setInfo, &texturesSet));

	for (auto& wd : writeDescriptorSets)
	{
		wd.dstSet = texturesSet;
	}


	if (writeDescriptorSets.size() > 0)
	{
		uint32_t texInfoIndex = 0;
		for (auto& wd : writeDescriptorSets)
		{
			if (wd.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				MaterialTexture textureId = static_cast<MaterialTexture>(wd.dstBinding);
				VulkanTexture::Pointer texture = renderer->defaultTexture();
				VulkanSampler::Pointer sampler = renderer->defaultSampler();
				textureInfoPool[texInfoIndex].imageLayout = texture->nativeTexture().layout;
				textureInfoPool[texInfoIndex].imageView = texture->nativeTexture().imageView;
				textureInfoPool[texInfoIndex].sampler = sampler->nativeSampler().sampler;
				wd.pImageInfo = textureInfoPool.data() + texInfoIndex;
				++texInfoIndex;
			}
		}
		vkUpdateDescriptorSets(vulkan.device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}
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
