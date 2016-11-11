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

	void generatePipelineLayout(const PipelineState::Reflection& reflection);

	VulkanRenderer* renderer = nullptr;
	VulkanState& vulkan;

	Vector<VkWriteDescriptorSet> writeDescriptorSets;
	Vector<VkDescriptorBufferInfo> bufferInfoPool;
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
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->descriptorSetLayouts[DescriptorSetClass::Buffers], nullptr);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->descriptorSetLayouts[DescriptorSetClass::Textures], nullptr);
	vkFreeDescriptorSets(_private->vulkan.device, _private->descriptprPool, DescriptorSetClass::Count, _private->descriptorSets);
	vkDestroyDescriptorPool(_private->vulkan.device, _private->descriptprPool, nullptr);
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
	
	_private->generatePipelineLayout(reflection);

	VkVertexInputBindingDescription binding = { };
	Vector<VkVertexInputAttributeDescription> attribs;
	VkPipelineVertexInputStateCreateInfo vertexInfo = { };
	_private->generateInputLayout(inputLayout(), vertexInfo, attribs, binding);

	VulkanRenderPass::Pointer pass = renderPass();

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

bool bbb = false;

void VulkanPipelineState::bind(VulkanNativeRenderPass& pass, MaterialInstance::Pointer& material)
{
	uint32_t objectBufferOffset = 0;
	if (reflection.objectVariablesBufferSize > 0)
	{
		uint8_t* dst = _private->renderer->sharedConstBuffer().allocateData(reflection.objectVariablesBufferSize, objectBufferOffset);
		memcpy(dst, objectVariablesBuffer.data(), reflection.objectVariablesBufferSize);
	}

	if (_private->writeDescriptorSets.size() > 0)
	{
		uint32_t bufferInfoIndex = 0;
		uint32_t texInfoIndex = 0;
		for (auto& wd : _private->writeDescriptorSets)
		{
			if (wd.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			{
				uint32_t offset = 0;
				uint32_t length = 0;
				VulkanDataBuffer::Pointer sharedBuffer;
				if (wd.dstBinding == ObjectVariablesBufferIndex)
				{
					sharedBuffer = _private->renderer->sharedConstBuffer().buffer();
					offset = objectBufferOffset;
					length = reflection.objectVariablesBufferSize;
				}
				else if (wd.dstBinding == MaterialVariablesBufferIndex)
				{
					length = reflection.materialVariablesBufferSize;
				}
				else if (wd.dstBinding == PassVariablesBufferIndex)
				{
					sharedBuffer = _private->renderer->sharedVariables().buffer();
					length = reflection.passVariablesBufferSize;
				}
				_private->bufferInfoPool[bufferInfoIndex].buffer = sharedBuffer->nativeBuffer().buffer();
				_private->bufferInfoPool[bufferInfoIndex].offset = offset;
				_private->bufferInfoPool[bufferInfoIndex].range = length;
				wd.pBufferInfo = _private->bufferInfoPool.data() + bufferInfoIndex;
				++bufferInfoIndex;
			}
			else if (wd.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				MaterialTexture textureId = static_cast<MaterialTexture>(wd.dstBinding);
				
				VulkanTexture::Pointer texture = material->texture(textureId);
				if (texture.invalid())
					texture = _private->renderer->defaultTexture();

				VulkanSampler::Pointer sampler = material->sampler(textureId);
				if (sampler.invalid())
					sampler = _private->renderer->defaultSampler();

				_private->textureInfoPool[texInfoIndex].imageLayout = texture->nativeTexture().layout;
				_private->textureInfoPool[texInfoIndex].imageView = texture->nativeTexture().imageView;
				_private->textureInfoPool[texInfoIndex].sampler = sampler->nativeSampler().sampler;
				wd.pImageInfo = _private->textureInfoPool.data() + texInfoIndex;
				++texInfoIndex;
			}
		}

		vkUpdateDescriptorSets(_private->vulkan.device, static_cast<uint32_t>(_private->writeDescriptorSets.size()), 
			_private->writeDescriptorSets.data(), 0, nullptr);
	}

	vkCmdBindPipeline(pass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _private->pipeline);
	
	vkCmdBindDescriptorSets(pass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _private->layout, 
		0, DescriptorSetClass::Count, _private->descriptorSets, 0, nullptr);
}

void VulkanPipelineStatePrivate::generatePipelineLayout(const PipelineState::Reflection& reflection)
{
	Vector<VkDescriptorSetLayoutBinding> bufferBindings;
	Vector<VkDescriptorSetLayoutBinding> textureBindings;

	bufferBindings.reserve(8);
	textureBindings.reserve(8);
	bufferInfoPool.resize(8);
	textureInfoPool.resize(8);

	auto addUniform = [&](uint32_t bindingIndex, VkDescriptorType descType) {
		auto& bindings = (descType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ? bufferBindings : textureBindings;

		bindings.emplace_back();
		bindings.back().binding = bindingIndex;
		bindings.back().stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings.back().descriptorType = descType;
		bindings.back().descriptorCount = 1;

		writeDescriptorSets.emplace_back();
		writeDescriptorSets.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets.back().descriptorCount = bindings.back().descriptorCount;
		writeDescriptorSets.back().descriptorType = bindings.back().descriptorType;
		writeDescriptorSets.back().dstBinding = bindings.back().binding;
	};
	 
	if (reflection.objectVariablesBufferSize > 0)
		addUniform(ObjectVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	if (reflection.materialVariablesBufferSize > 0)
		addUniform(MaterialVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	if (reflection.passVariablesBufferSize > 0)
		addUniform(PassVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	for (const auto& tex : reflection.fragmentTextures)
		addUniform(tex.second, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	{
		VkDescriptorSetLayoutCreateInfo buffersSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		buffersSetInfo.bindingCount = static_cast<uint32_t>(bufferBindings.size());
		buffersSetInfo.pBindings = bufferBindings.data();
		VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &buffersSetInfo, nullptr, descriptorSetLayouts + DescriptorSetClass::Buffers));

		VkDescriptorSetLayoutCreateInfo texturesSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		texturesSetInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
		texturesSetInfo.pBindings = textureBindings.data();
		VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &texturesSetInfo, nullptr, descriptorSetLayouts + DescriptorSetClass::Textures));
	}

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutInfo.pSetLayouts = descriptorSetLayouts;
	layoutInfo.setLayoutCount = DescriptorSetClass::Count;
	VULKAN_CALL(vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &layout));
	
	VkDescriptorPoolSize poolSizes[2] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(bufferBindings.size()) },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(textureBindings.size()) }
	};

	VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = DescriptorSetClass::Count;
	poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
	poolInfo.pPoolSizes = poolSizes;
	VULKAN_CALL(vkCreateDescriptorPool(vulkan.device, &poolInfo, nullptr, &descriptprPool));

	VkDescriptorSetAllocateInfo setInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	setInfo.descriptorSetCount = DescriptorSetClass::Count;
	setInfo.pSetLayouts = descriptorSetLayouts;
	setInfo.descriptorPool = descriptprPool;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &setInfo, descriptorSets));

	for (auto& wd : writeDescriptorSets)
	{
		if (wd.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			wd.dstSet = descriptorSets[DescriptorSetClass::Buffers];
		else if (wd.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			wd.dstSet = descriptorSets[DescriptorSetClass::Textures];
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
