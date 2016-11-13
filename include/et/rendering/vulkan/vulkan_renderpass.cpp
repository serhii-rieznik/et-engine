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
	VkBuffer vertexBuffers[1] { };
	VkDeviceSize vertexBuffersOffset[1] { };
	VkBuffer indexBuffer = nullptr;
	VkPipeline pipeline = nullptr;
	VkDescriptorSet	descriptorSets[DescriptorSetClass::Count] { };
	uint32_t descriptorSetOffsets[DescriptorSetClass::Count] { };
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
	
	RenderPass::Variables* variablesData = nullptr;
	uint32_t variablesDataBufferOffset = 0;
	Vector<VulkanRenderBatch> pendingBatches;

	void generateDynamicDescriptorSet(RenderPass* pass);
	void loadVariables(Camera::Pointer camera, Camera::Pointer light);
};

VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo) 
	: RenderPass(renderer, passInfo)
{
	ET_PIMPL_INIT(VulkanRenderPass, vulkan, renderer);

	uint8_t* dataPtr = dynamicConstantBuffer().staticAllocate(sizeof(Variables), _private->variablesDataBufferOffset);
	_private->variablesData = reinterpret_cast<RenderPass::Variables*>(dataPtr);
	_private->pendingBatches.reserve(128);
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
	vkDestroyFramebuffer(_private->vulkan.device, _private->framebuffer, nullptr);
	vkDestroyRenderPass(_private->vulkan.device, _private->renderPass, nullptr);
	vkFreeCommandBuffers(_private->vulkan.device, _private->vulkan.commandPool, 1, &_private->commandBuffer);
	vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptprPool, 1, &_private->dynamicDescriptorSet);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->dynamicDescriptorSetLayout, nullptr);
	
	dynamicConstantBuffer().free(reinterpret_cast<uint8_t*>(_private->variablesData));

	ET_PIMPL_FINALIZE(VulkanRenderPass)
}

const VulkanNativeRenderPass& VulkanRenderPass::nativeRenderPass() const
{
	return *(_private);
}

void VulkanRenderPass::begin()
{
	VkImageView attachments[] = {
		_private->vulkan.swapchain.currentRenderTarget().colorView,
		_private->vulkan.swapchain.currentRenderTarget().depthView
	};

	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.width = _private->vulkan.swapchain.extent.width;
	framebufferInfo.height = _private->vulkan.swapchain.extent.height;
	framebufferInfo.layers = 1;
	framebufferInfo.renderPass = _private->renderPass;
	VULKAN_CALL(vkCreateFramebuffer(_private->vulkan.device, &framebufferInfo, nullptr, &_private->framebuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VULKAN_CALL(vkBeginCommandBuffer(_private->commandBuffer, &commandBufferBeginInfo));

	vulkan::imageBarrier(_private->vulkan, _private->commandBuffer, _private->vulkan.swapchain.currentColorImage(), 
		VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.clearValueCount = sizeof(_private->clearValues) / sizeof(_private->clearValues[0]);
	renderPassBeginInfo.pClearValues = _private->clearValues;
	renderPassBeginInfo.renderPass = _private->renderPass;
	renderPassBeginInfo.renderArea.extent = _private->vulkan.swapchain.extent;
	renderPassBeginInfo.framebuffer = _private->framebuffer;
	vkCmdBeginRenderPass(_private->commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	_private->scissor.extent.width = static_cast<uint32_t>(_private->renderer->rc()->size().x);
	_private->scissor.extent.height = static_cast<uint32_t>(_private->renderer->rc()->size().y);
	_private->viewport.width = static_cast<float>(_private->renderer->rc()->size().x);
	_private->viewport.height = static_cast<float>(_private->renderer->rc()->size().y);
	_private->viewport.maxDepth = 1.0f;
	
	_private->loadVariables(info().camera, info().light);

	vkCmdSetScissor(_private->commandBuffer, 0, 1, &_private->scissor);
	vkCmdSetViewport(_private->commandBuffer, 0, 1, &_private->viewport);
}

void VulkanRenderPass::validateRenderBatch(RenderBatch::Pointer batch)
{
	VulkanRenderPass::Pointer vulkanRenderPass(this);
	_private->renderer->acquirePipelineState(vulkanRenderPass, batch->material(), batch->vertexStream());
	batch->material()->textureSet();
	batch->material()->sharedConstantBufferOffset();
}

void VulkanRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	VulkanRenderPass::Pointer vulkanRenderPass(this);

	VulkanPipelineState::Pointer ps = _private->renderer->acquirePipelineState(vulkanRenderPass, batch->material(), batch->vertexStream());
	ps->setObjectVariable(PipelineState::kWorldTransform(), batch->transformation());
	ps->setObjectVariable(PipelineState::kWorldRotationTransform(), batch->rotationTransformation());
	ps->bind(vulkanRenderPass, batch->material());

	VkCommandBuffer cmd = _private->commandBuffer;
	VulkanVertexBuffer::Pointer vb = batch->vertexStream()->vertexBuffer();
	VulkanIndexBuffer::Pointer ib = batch->vertexStream()->indexBuffer();
	VkDeviceSize vOffsets[1] = { 0 };
	VkBuffer vBuffers[1] = { vb->nativeBuffer().buffer() };
	vkCmdBindVertexBuffers(cmd, 0, 1, vBuffers, vOffsets );
	vkCmdBindIndexBuffer(cmd, ib->nativeBuffer().buffer(), 0, vulkan::indexBufferFormat(ib->format()));
	
	vkCmdDrawIndexed(cmd, batch->numIndexes(), 1, batch->firstIndex(), 0, 0);
}

void VulkanRenderPass::end()
{
	vkCmdEndRenderPass(_private->commandBuffer);

	vulkan::imageBarrier(_private->vulkan, _private->commandBuffer, _private->vulkan.swapchain.currentColorImage(), 
		VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	VULKAN_CALL(vkEndCommandBuffer(_private->commandBuffer));
}

void VulkanRenderPass::submit()
{
	_private->renderer->sharedConstantBuffer().flush();
	dynamicConstantBuffer().flush();

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
	VkDescriptorBufferInfo passBufferInfo = { db->nativeBuffer().buffer(), variablesDataBufferOffset, sizeof(RenderPass::Variables) };
	VkDescriptorBufferInfo objectBufferInfo = { db->nativeBuffer().buffer(), 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo materialBufferInfo = { sb->nativeBuffer().buffer(), 0, VK_WHOLE_SIZE };
	VkDescriptorSetLayoutBinding bindings[] = { { }, {  }, {  } };
	VkWriteDescriptorSet writeSets[] = { { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }, { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }, { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET } };
	{
		bindings[0] = { ObjectVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 };
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		writeSets[0].descriptorCount = bindings[0].descriptorCount;
		writeSets[0].descriptorType = bindings[0].descriptorType;
		writeSets[0].dstBinding = bindings[0].binding;
		writeSets[0].pBufferInfo = &objectBufferInfo;
		
		bindings[1] = { MaterialVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 };
		bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		writeSets[1].descriptorCount = bindings[1].descriptorCount;
		writeSets[1].descriptorType = bindings[1].descriptorType;
		writeSets[1].dstBinding = bindings[1].binding;
		writeSets[1].pBufferInfo = &materialBufferInfo;

		bindings[2] = { PassVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
		bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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
	if (camera.valid())
	{
		variablesData->viewProjection = camera->viewProjectionMatrix();
		variablesData->projection = camera->projectionMatrix();
		variablesData->view = camera->viewMatrix();
		variablesData->cameraPosition = vec4(camera->position());
		variablesData->cameraDirection = vec4(camera->direction());
		variablesData->cameraUp = vec4(camera->up());
	}

	if (light.valid())
	{
		variablesData->lightPosition = vec4(light->position());
	}
}

}
