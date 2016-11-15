/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

struct VulkanRenderBatch
{
	VkBuffer indexBuffer = nullptr;
	VkIndexType indexBufferFormat = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;
	uint32_t startIndex = 0;
	uint32_t indexCount = 0;

	VkBuffer vertexBuffers[1] { };
	VkDeviceSize vertexBuffersOffset[1] { };

	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	ConstantBufferEntry objectVariables;
	ConstantBufferEntry materialVariables;
	VkDescriptorSet	descriptorSets[DescriptorSetClass::Count] { };
	uint32_t dynamicOffsets[DescriptorSetClass::DynamicDescriptorsCount] { };
};

class VulkanRenderPassPrivate : public VulkanNativeRenderPass
{
public:
	VulkanRenderPassPrivate(VulkanState& v, VulkanRenderer* r) 
		: vulkan(v), renderer(r) { }

	VulkanState& vulkan;
	VkFence fence = nullptr;
	VulkanRenderer* renderer = nullptr;
	VkClearValue clearValues[2] { };
	VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	
	ConstantBufferEntry variablesData;
	Vector<VulkanRenderBatch> batches;
	Map<VkImageView, VkFramebuffer> framebuffers;

	std::atomic_bool recording { false };

	void generateDynamicDescriptorSet(RenderPass* pass);
	void loadVariables(Camera::Pointer camera, Camera::Pointer light);
};

VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo) 
	: RenderPass(renderer, passInfo)
{
	ET_PIMPL_INIT(VulkanRenderPass, vulkan, renderer);

	_private->variablesData = dynamicConstantBuffer().staticAllocate(sizeof(Variables));
	_private->batches.reserve(128);
	_private->generateDynamicDescriptorSet(this);

	VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VULKAN_CALL(vkCreateFence(vulkan.device, &fenceInfo, nullptr, &_private->fence));

	_private->clearValues[0] = { 
		passInfo.target.clearColor.x, 
		passInfo.target.clearColor.y, 
		passInfo.target.clearColor.z, 
		passInfo.target.clearColor.w
	};
	_private->clearValues[1] = {
		passInfo.target.clearDepth
	};

	VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	info.commandPool = vulkan.commandPool;
	info.commandBufferCount = 1;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	
	VULKAN_CALL(vkAllocateCommandBuffers(vulkan.device, &info, &_private->commandBuffer));	

	VkAttachmentDescription attachments[2] = { };
	attachments[0].format = vulkan.swapchain.surfaceFormat.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[1].format = vulkan.swapchain.depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassInfo = { };
	subpassInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassInfo.colorAttachmentCount = 1;
	subpassInfo.pColorAttachments = &colorReference;
	subpassInfo.pDepthStencilAttachment = &depthReference;

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassInfo;

	VULKAN_CALL(vkCreateRenderPass(vulkan.device, &createInfo, nullptr, &_private->renderPass));
}

VulkanRenderPass::~VulkanRenderPass()
{
	for (const auto& fb : _private->framebuffers)
		vkDestroyFramebuffer(_private->vulkan.device, fb.second, nullptr);

	vkDestroyRenderPass(_private->vulkan.device, _private->renderPass, nullptr);
	vkFreeCommandBuffers(_private->vulkan.device, _private->vulkan.commandPool, 1, &_private->commandBuffer);
	vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptprPool, 1, &_private->dynamicDescriptorSet);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->dynamicDescriptorSetLayout, nullptr);
	
	dynamicConstantBuffer().free(_private->variablesData);

	ET_PIMPL_FINALIZE(VulkanRenderPass)
}

const VulkanNativeRenderPass& VulkanRenderPass::nativeRenderPass() const
{
	return *(_private);
}

void VulkanRenderPass::begin()
{
	ET_ASSERT(_private->recording == false);

	const VulkanSwapchain::RenderTarget& currentRenderTarget = _private->vulkan.swapchain.currentRenderTarget();
	VkFramebuffer currentFramebuffer = _private->framebuffers[currentRenderTarget.colorView];

	if (currentFramebuffer == nullptr)
	{
		VkImageView attachments[] = { currentRenderTarget.colorView, currentRenderTarget.depthView };
		VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = _private->vulkan.swapchain.extent.width;
		framebufferInfo.height = _private->vulkan.swapchain.extent.height;
		framebufferInfo.layers = 1;
		framebufferInfo.renderPass = _private->renderPass;
		VULKAN_CALL(vkCreateFramebuffer(_private->vulkan.device, &framebufferInfo, nullptr, &currentFramebuffer));
		
		_private->framebuffers[currentRenderTarget.colorView] = currentFramebuffer;
	}

	_private->beginInfo.clearValueCount = sizeof(_private->clearValues) / sizeof(_private->clearValues[0]);
	_private->beginInfo.pClearValues = _private->clearValues;
	_private->beginInfo.renderPass = _private->renderPass;
	_private->beginInfo.renderArea.extent = _private->vulkan.swapchain.extent;
	_private->beginInfo.framebuffer = currentFramebuffer;
	
	_private->scissor.extent.width = static_cast<uint32_t>(_private->renderer->rc()->size().x);
	_private->scissor.extent.height = static_cast<uint32_t>(_private->renderer->rc()->size().y);
	_private->viewport.width = static_cast<float>(_private->renderer->rc()->size().x);
	_private->viewport.height = static_cast<float>(_private->renderer->rc()->size().y);
	_private->viewport.maxDepth = 1.0f;
	_private->loadVariables(info().camera, info().light);

	_private->recording = true;
}

void VulkanRenderPass::pushRenderBatch(const RenderBatch::Pointer& inBatch)
{
	ET_ASSERT(_private->recording);

	VulkanRenderPass::Pointer vulkanRenderPass(this);
	MaterialInstance::Pointer batchMaterial = inBatch->material();
	VulkanPipelineState::Pointer ps = _private->renderer->acquirePipelineState(vulkanRenderPass, batchMaterial, inBatch->vertexStream());
	
	const VulkanProgram::Pointer& program = batchMaterial->program();
	const VulkanVertexBuffer::Pointer& vb = inBatch->vertexStream()->vertexBuffer();
	const VulkanIndexBuffer::Pointer& ib = inBatch->vertexStream()->indexBuffer();
	const VulkanTextureSet::Pointer& textureSet = batchMaterial->textureSet();

	ConstantBufferEntry objectVariables;
	if (program->reflection().objectVariablesBufferSize > 0)
	{
		objectVariables = dynamicConstantBuffer().dynamicAllocate(program->reflection().objectVariablesBufferSize);
		auto var = program->reflection().objectVariables.find(PipelineState::kWorldTransform());
		if (var != program->reflection().objectVariables.end())
		{
			memcpy(objectVariables.data() + var->second.offset, inBatch->transformation().data(), sizeof(inBatch->transformation()));
		}
		var = program->reflection().objectVariables.find(PipelineState::kWorldRotationTransform());
		if (var != program->reflection().objectVariables.end())
		{
			memcpy(objectVariables.data() + var->second.offset, inBatch->rotationTransformation().data(), sizeof(inBatch->rotationTransformation()));
		}
	}

	_private->batches.emplace_back();
	VulkanRenderBatch& batch = _private->batches.back();
	batch.indexBuffer = ib->nativeBuffer().buffer();
	batch.indexBufferFormat = vulkan::indexBufferFormat(ib->format());
	batch.startIndex = inBatch->firstIndex();
	batch.indexCount = inBatch->numIndexes();
	batch.vertexBuffers[0] =  vb->nativeBuffer().buffer();
	batch.vertexBuffersOffset[0] = 0;
	batch.pipeline = ps->nativePipeline().pipeline;
	batch.pipelineLayout = ps->nativePipeline().layout;
	batch.descriptorSets[0] = _private->dynamicDescriptorSet;
	batch.descriptorSets[1] = textureSet->nativeSet().descriptorSet;
	batch.dynamicOffsets[0] = objectVariables.offset();
	batch.dynamicOffsets[1] = batchMaterial->constantBufferData().offset();
}

void VulkanRenderPass::end()
{
	_private->recording = false;
}

void VulkanRenderPass::submit()
{
	ET_ASSERT(_private->recording == false);

	_private->renderer->sharedConstantBuffer().flush();
	dynamicConstantBuffer().flush();

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VULKAN_CALL(vkBeginCommandBuffer(_private->commandBuffer, &commandBufferBeginInfo));
	vulkan::imageBarrier(_private->vulkan, _private->commandBuffer, _private->vulkan.swapchain.currentColorImage(), 
		VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	vkCmdSetScissor(_private->commandBuffer, 0, 1, &_private->scissor);
	vkCmdSetViewport(_private->commandBuffer, 0, 1, &_private->viewport);

	VkPipeline lastPipeline = nullptr;
	VkBuffer lastVertexBuffer = nullptr;
	VkBuffer lastIndexBuffer = nullptr;
	VkIndexType lastIndexType = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;
	VkDeviceSize lastVertexBufferOffset = InvalidIndex;

	vkCmdBeginRenderPass(_private->commandBuffer, &_private->beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	for (const auto& batch : _private->batches)
	{
		if (batch.pipeline != lastPipeline)
		{
			vkCmdBindPipeline(_private->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.pipeline);
			lastPipeline = batch.pipeline;
		}
		
		if ((batch.vertexBuffers[0] != lastVertexBuffer) || (batch.vertexBuffersOffset[0] != lastVertexBufferOffset))
		{
			vkCmdBindVertexBuffers(_private->commandBuffer, 0, 1, batch.vertexBuffers, batch.vertexBuffersOffset);
			lastVertexBuffer = batch.vertexBuffers[0];
			lastVertexBufferOffset = batch.vertexBuffersOffset[0];
		}
		
		if ((batch.indexBuffer != lastIndexBuffer) || (batch.indexBufferFormat != lastIndexType))
		{
			vkCmdBindIndexBuffer(_private->commandBuffer, batch.indexBuffer, 0, batch.indexBufferFormat);
			lastIndexBuffer = batch.indexBuffer;
			lastIndexType = batch.indexBufferFormat;
		}
		
		vkCmdBindDescriptorSets(_private->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.pipelineLayout, 0, 
			DescriptorSetClass::Count, batch.descriptorSets, DescriptorSetClass::DynamicDescriptorsCount, batch.dynamicOffsets);
		
		vkCmdDrawIndexed(_private->commandBuffer, batch.indexCount, 1, batch.startIndex, 0, 0);
	}
	vkCmdEndRenderPass(_private->commandBuffer);
	vulkan::imageBarrier(_private->vulkan, _private->commandBuffer, _private->vulkan.swapchain.currentColorImage(),
		VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	VULKAN_CALL(vkEndCommandBuffer(_private->commandBuffer));

	_private->batches.clear();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_private->commandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_private->vulkan.semaphores.imageAvailable;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &_private->vulkan.semaphores.renderComplete;
	submitInfo.pWaitDstStageMask = waitStages;

	VULKAN_CALL(vkQueueSubmit(_private->vulkan.queue, 1, &submitInfo, _private->fence));
	VULKAN_CALL(vkWaitForFences(_private->vulkan.device, 1, &_private->fence, VK_TRUE, ~0ull));
	VULKAN_CALL(vkResetFences(_private->vulkan.device, 1, &_private->fence));
}

void VulkanRenderPassPrivate::generateDynamicDescriptorSet(RenderPass* pass)
{
	VulkanDataBuffer::Pointer db = pass->dynamicConstantBuffer().buffer();
	VulkanDataBuffer::Pointer sb = renderer->sharedConstantBuffer().buffer();
	VkDescriptorBufferInfo passBufferInfo = { db->nativeBuffer().buffer(), variablesData.offset(), sizeof(RenderPass::Variables) };
	VkDescriptorBufferInfo objectBufferInfo = { db->nativeBuffer().buffer(), 0, VK_WHOLE_SIZE }; // TODO : calculate offset and size
	VkDescriptorBufferInfo materialBufferInfo = { sb->nativeBuffer().buffer(), 0, VK_WHOLE_SIZE }; // TODO : calculate offset and size
	VkDescriptorSetLayoutBinding bindings[] = { { }, {  }, {  } };
	VkWriteDescriptorSet writeSets[] = { { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }, { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }, { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET } };
	{
		bindings[0] = { ObjectVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 };
		bindings[0].stageFlags = VK_SHADER_STAGE_ALL;
		writeSets[0].descriptorCount = bindings[0].descriptorCount;
		writeSets[0].descriptorType = bindings[0].descriptorType;
		writeSets[0].dstBinding = bindings[0].binding;
		writeSets[0].pBufferInfo = &objectBufferInfo;
		
		bindings[1] = { MaterialVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 };
		bindings[1].stageFlags = VK_SHADER_STAGE_ALL;
		writeSets[1].descriptorCount = bindings[1].descriptorCount;
		writeSets[1].descriptorType = bindings[1].descriptorType;
		writeSets[1].dstBinding = bindings[1].binding;
		writeSets[1].pBufferInfo = &materialBufferInfo;

		bindings[2] = { PassVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
		bindings[2].stageFlags = VK_SHADER_STAGE_ALL;
		writeSets[2].descriptorCount = bindings[2].descriptorCount;
		writeSets[2].descriptorType = bindings[2].descriptorType;
		writeSets[2].dstBinding = bindings[2].binding;
		writeSets[2].pBufferInfo = &passBufferInfo;
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetLayoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
	descriptorSetLayoutCreateInfo.pBindings = bindings;
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &descriptorSetLayoutCreateInfo, nullptr, &dynamicDescriptorSetLayout));

	VkDescriptorSetAllocateInfo descriptorAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorAllocInfo.pSetLayouts = &dynamicDescriptorSetLayout;
	descriptorAllocInfo.descriptorPool = vulkan.descriptprPool;
	descriptorAllocInfo.descriptorSetCount = 1;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &descriptorAllocInfo, &dynamicDescriptorSet));

	for (VkWriteDescriptorSet& wd : writeSets)
		wd.dstSet = dynamicDescriptorSet;
	
	uint32_t writeSetsCount = static_cast<uint32_t>(sizeof(writeSets) / sizeof(writeSets[0]));
	vkUpdateDescriptorSets(vulkan.device, writeSetsCount, writeSets, 0, nullptr);
}

void VulkanRenderPassPrivate::loadVariables(Camera::Pointer camera, Camera::Pointer light)
{
	RenderPass::Variables* vptr = reinterpret_cast<RenderPass::Variables*>(variablesData.data());
	if (camera.valid())
	{
		vptr->viewProjection = camera->viewProjectionMatrix();
		vptr->projection = camera->projectionMatrix();
		vptr->view = camera->viewMatrix();
		vptr->cameraPosition = vec4(camera->position());
		vptr->cameraDirection = vec4(camera->direction());
		vptr->cameraUp = vec4(camera->up());
	}

	if (light.valid())
	{
		vptr->lightPosition = vec4(light->position());
	}
}

}
