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

class VulkanRenderPassPrivate
{
public:
	VulkanRenderPassPrivate(VulkanState& v, VulkanRenderer* r) 
		: vulkan(v), renderer(r) { }

	VulkanState& vulkan;
	VulkanNativeRenderPass nativePass;
	VkFence fence = nullptr;
	VulkanRenderer* renderer = nullptr;
	VkClearValue clearValues[2] { };
};

VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo) 
	: RenderPass(passInfo)
{
	ET_PIMPL_INIT(VulkanRenderPass, vulkan, renderer);

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
	VULKAN_CALL(vkAllocateCommandBuffers(vulkan.device, &info, &_private->nativePass.commandBuffer));	

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

	VULKAN_CALL(vkCreateRenderPass(vulkan.device, &createInfo, nullptr, &_private->nativePass.renderPass));
}

VulkanRenderPass::~VulkanRenderPass()
{
	vkDestroyFramebuffer(_private->vulkan.device, _private->nativePass.framebuffer, nullptr);
	vkDestroyRenderPass(_private->vulkan.device, _private->nativePass.renderPass, nullptr);
	vkFreeCommandBuffers(_private->vulkan.device, _private->vulkan.commandPool, 1, &_private->nativePass.commandBuffer);
	ET_PIMPL_FINALIZE(VulkanRenderPass)
}

const VulkanNativeRenderPass& VulkanRenderPass::nativeRenderPass() const
{
	return _private->nativePass;
}

void VulkanRenderPass::submit()
{
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_private->nativePass.commandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_private->vulkan.semaphores.imageAvailable;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &_private->vulkan.semaphores.renderComplete;
	submitInfo.pWaitDstStageMask = waitStages;

	VULKAN_CALL(vkQueueSubmit(_private->vulkan.queue, 1, &submitInfo, _private->fence));
	VULKAN_CALL(vkWaitForFences(_private->vulkan.device, 1, &_private->fence, VK_TRUE, ~0ull));
	VULKAN_CALL(vkResetFences(_private->vulkan.device, 1, &_private->fence));
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
	framebufferInfo.renderPass = _private->nativePass.renderPass;
	VULKAN_CALL(vkCreateFramebuffer(_private->vulkan.device, &framebufferInfo, nullptr, &_private->nativePass.framebuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(_private->nativePass.commandBuffer, &commandBufferBeginInfo);

	vulkan::imageBarrier(_private->vulkan, _private->nativePass.commandBuffer, _private->vulkan.swapchain.currentColorImage(), 
		VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.clearValueCount = sizeof(_private->clearValues) / sizeof(_private->clearValues[0]);
	renderPassBeginInfo.pClearValues = _private->clearValues;
	renderPassBeginInfo.renderPass = _private->nativePass.renderPass;
	renderPassBeginInfo.renderArea.extent = _private->vulkan.swapchain.extent;
	renderPassBeginInfo.framebuffer = _private->nativePass.framebuffer;
	vkCmdBeginRenderPass(_private->nativePass.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	_private->nativePass.scissor.extent.width = static_cast<uint32_t>(_private->renderer->rc()->size().x);
	_private->nativePass.scissor.extent.height = static_cast<uint32_t>(_private->renderer->rc()->size().y);
	_private->nativePass.viewport.width = static_cast<float>(_private->renderer->rc()->size().x);
	_private->nativePass.viewport.height = static_cast<float>(_private->renderer->rc()->size().y);
	_private->nativePass.viewport.maxDepth = 1.0f;
}

void VulkanRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	VulkanRenderPass::Pointer vulkanRenderPass(this);

	SharedVariables& sharedVariables = _private->renderer->sharedVariables();
	if (info().camera.valid())
		sharedVariables.loadCameraProperties(info().camera);
	if (info().light.valid())
		sharedVariables.loadLightProperties(info().light);

	VulkanPipelineState::Pointer ps = _private->renderer->acquirePipelineState(vulkanRenderPass, batch->material(), batch->vertexStream());
	ps->setObjectVariable(PipelineState::kWorldTransform(), batch->transformation());
	ps->setObjectVariable(PipelineState::kWorldRotationTransform(), batch->rotationTransformation());
	ps->bind(_private->nativePass, batch->material());

	VkCommandBuffer cmd = _private->nativePass.commandBuffer;
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
	_private->renderer->sharedVariables().flushBuffer();
	_private->renderer->sharedConstBuffer().flush();

	vkCmdEndRenderPass(_private->nativePass.commandBuffer);

	vulkan::imageBarrier(_private->vulkan, _private->nativePass.commandBuffer, _private->vulkan.swapchain.currentColorImage(), 
		VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	vkEndCommandBuffer(_private->nativePass.commandBuffer);
}

}
