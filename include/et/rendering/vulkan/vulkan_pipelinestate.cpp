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
#include <et/rendering/vulkan/vulkan_renderer.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanPipelineStatePrivate
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
	VulkanNativePipeline nativePipeline;

	VkDescriptorPool descriptprPool = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	Vector<VkWriteDescriptorSet> writeDescriptorSets;
	Vector<VkDescriptorBufferInfo> bufferInfoPool;

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
	vkDestroyPipeline(_private->vulkan.device, _private->nativePipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(_private->vulkan.device, _private->nativePipeline.layout, nullptr);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->nativePipeline.descriptorSetLayout, nullptr);
	vkFreeDescriptorSets(_private->vulkan.device, _private->descriptprPool, 1, &_private->descriptorSet);
	vkDestroyDescriptorPool(_private->vulkan.device, _private->descriptprPool, nullptr);
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
	
	VulkanProgram::Pointer prog = program();
	reflection = prog->reflection();
	buildBuffers();

	printReflection();
	
	_private->generatePipelineLayout(reflection);

	VkVertexInputBindingDescription binding = { };
	Vector<VkVertexInputAttributeDescription> attribs;
	VkPipelineVertexInputStateCreateInfo vertexInfo = { };
	_private->generateInputLayout(inputLayout(), vertexInfo, attribs, binding);

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO }; 
	viewportState.scissorCount = 1;
	viewportState.viewportCount = 1;

	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	{
		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
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
		info.layout = _private->nativePipeline.layout;
		info.renderPass = pass->nativeRenderPass().renderPass;
		info.pStages = prog->shaderModules().stageCreateInfo;
		info.stageCount = 2;
	}
	VULKAN_CALL(vkCreateGraphicsPipelines(_private->vulkan.device, _private->vulkan.pipelineCache, 1, &info,
		nullptr, &_private->nativePipeline.pipeline));
}

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
		}

		vkUpdateDescriptorSets(_private->vulkan.device, static_cast<uint32_t>(_private->writeDescriptorSets.size()), 
			_private->writeDescriptorSets.data(), 0, nullptr);
	}

	vkCmdBindPipeline(pass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _private->nativePipeline.pipeline);
	vkCmdBindDescriptorSets(pass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _private->nativePipeline.layout, 0, 1, 
		&_private->descriptorSet, 0, nullptr);
	vkCmdSetScissor(pass.commandBuffer, 0, 1, &pass.scissor);
	vkCmdSetViewport(pass.commandBuffer, 0, 1, &pass.viewport);

	/*
	for (const auto& prop : material->usedProperties())
	{
		uploadMaterialVariable(prop.first, prop.second.data, prop.second.size);
	}

	MetalDataBuffer::Pointer sharedBuffer = _private->renderer->sharedConstBuffer().buffer();
	id<MTLBuffer> mtlSharedBuffer = sharedBuffer->nativeBuffer().buffer();

	if (_private->buildMaterialBuffer)
	{
		uint32_t materialBufferOffset = 0;
		uint8_t* dst = _private->renderer->sharedConstBuffer().allocateData(_private->materialVariablesBufferSize, materialBufferOffset);
		memcpy(dst, _private->materialVariablesBuffer.data(), _private->materialVariablesBufferSize);
		if (_private->bindMaterialVariablesToVertex)
			[e.encoder setVertexBuffer:mtlSharedBuffer offset:materialBufferOffset atIndex:MaterialVariablesBufferIndex];
		if (_private->bindMaterialVariablesToFragment)
			[e.encoder setFragmentBuffer:mtlSharedBuffer offset:materialBufferOffset atIndex:MaterialVariablesBufferIndex];
	}

	for (const auto& vt : reflection.vertexTextures)
	{
		auto i = material->usedTextures().find(vt.first);
		MetalTexture::Pointer tex;
		if (i == material->usedTextures().end())
			tex = _private->renderer->defaultTexture();
		else
			tex = i->second.texture;
		[e.encoder setVertexTexture:tex->nativeTexture().texture atIndex:vt.second];
	}

	for (const auto& ft : reflection.fragmentTextures)
	{
		auto i = material->usedTextures().find(ft.first);
		MetalTexture::Pointer tex;
		if (i == material->usedTextures().end())
			tex = _private->renderer->defaultTexture();
		else
			tex = i->second.texture;
		[e.encoder setFragmentTexture:tex->nativeTexture().texture atIndex:ft.second];
	}

	for (const auto& vs : reflection.vertexSamplers)
	{
		auto i = material->usedSamplers().find(vs.first);
		MetalSampler::Pointer smp = (i == material->usedSamplers().end()) ?
			_private->renderer->defaultSampler() : i->second.sampler;
		[e.encoder setVertexSamplerState:smp->nativeSampler().sampler atIndex:vs.second];
	}

	for (const auto& fs : reflection.fragmentSamplers)
	{
		auto i = material->usedSamplers().find(fs.first);
		MetalSampler::Pointer smp = (i == material->usedSamplers().end()) ?
			_private->renderer->defaultSampler() : i->second.sampler;
		[e.encoder setFragmentSamplerState:smp->nativeSampler().sampler atIndex:fs.second];
	}

	[e.encoder setDepthStencilState:_private->state.depthStencilState];
	[e.encoder setRenderPipelineState:_private->state.pipelineState];
	*/
}

void VulkanPipelineStatePrivate::generatePipelineLayout(const PipelineState::Reflection& reflection)
{
	Vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(8);
	
	Vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(8);

	bufferInfoPool.resize(8);

	auto addUniformBuffer = [&](uint32_t bindingIndex){
		bindings.emplace_back();
		VkDescriptorSetLayoutBinding& binding = bindings.back();
		binding.binding = bindingIndex;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.descriptorCount = 1;

		objectVariablesIndex = static_cast<uint32_t>(poolSizes.size());
		poolSizes.emplace_back();
		poolSizes.back().type = binding.descriptorType;
		poolSizes.back().descriptorCount = binding.descriptorCount;

		writeDescriptorSets.emplace_back();
		writeDescriptorSets.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets.back().descriptorCount = binding.descriptorCount;
		writeDescriptorSets.back().descriptorType = binding.descriptorType;
		writeDescriptorSets.back().dstBinding = binding.binding;
	};
	 
	if (reflection.objectVariablesBufferSize > 0)
		addUniformBuffer(ObjectVariablesBufferIndex);

	if (reflection.materialVariablesBufferSize > 0)
		addUniformBuffer(MaterialVariablesBufferIndex);

	if (reflection.passVariablesBufferSize > 0)
		addUniformBuffer(PassVariablesBufferIndex);

	// TODO : add textures

	if (bindings.empty())
		return;

	VkDescriptorSetLayoutCreateInfo descriptorSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetInfo.pBindings = bindings.data();
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &descriptorSetInfo, nullptr, &nativePipeline.descriptorSetLayout));

	VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutInfo.pSetLayouts = &nativePipeline.descriptorSetLayout;
	layoutInfo.setLayoutCount = 1;
	VULKAN_CALL(vkCreatePipelineLayout(vulkan.device, &layoutInfo, nullptr, &nativePipeline.layout));
	
	VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	VULKAN_CALL(vkCreateDescriptorPool(vulkan.device, &poolInfo, nullptr, &descriptprPool));

	VkDescriptorSetAllocateInfo setInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	setInfo.descriptorSetCount = 1;
	setInfo.pSetLayouts = &nativePipeline.descriptorSetLayout;
	setInfo.descriptorPool = descriptprPool;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &setInfo, &descriptorSet));

	for (auto& wd : writeDescriptorSets)
	{
		wd.dstSet = descriptorSet;
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
