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
};

VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo) 
	: RenderPass(passInfo)
{
	ET_PIMPL_INIT(VulkanRenderPass, vulkan, renderer);

	VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VULKAN_CALL(vkCreateFence(vulkan.device, &fenceInfo, nullptr, &_private->fence));

	_private->nativePass.viewport.width = static_cast<float>(_private->vulkan.swapchain.extent.width);
	_private->nativePass.viewport.height = static_cast<float>(_private->vulkan.swapchain.extent.height);
	_private->nativePass.viewport.maxDepth = 1.0f;

	_private->nativePass.scissor.extent.width = _private->vulkan.swapchain.extent.width;
	_private->nativePass.scissor.extent.height = _private->vulkan.swapchain.extent.height;

	VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	info.commandPool = vulkan.commandPool;
	info.commandBufferCount = 1;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	
	VULKAN_CALL(vkAllocateCommandBuffers(vulkan.device, &info, &_private->nativePass.commandBuffer));	

	VkAttachmentDescription attachments[1] = { };
	attachments[0].format = vulkan.swapchain.surfaceFormat.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { };
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassInfo = { };
	subpassInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassInfo.colorAttachmentCount = 1;									// Subpass uses one color attachment
	subpassInfo.pColorAttachments = &colorReference;							// Reference to the color attachment in slot 0

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassInfo;

	VULKAN_CALL(vkCreateRenderPass(vulkan.device, &createInfo, nullptr, &_private->nativePass.renderPass));

	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &_private->vulkan.swapchain.images.at(_private->vulkan.swapchain.currentImageIndex).imageView;
	framebufferInfo.width = _private->vulkan.swapchain.extent.width;
	framebufferInfo.height = _private->vulkan.swapchain.extent.height;
	framebufferInfo.layers = 1;
	framebufferInfo.renderPass = _private->nativePass.renderPass;
	VULKAN_CALL(vkCreateFramebuffer(vulkan.device, &framebufferInfo, nullptr, &_private->nativePass.framebuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(_private->nativePass.commandBuffer, &commandBufferBeginInfo);

	vulkan::imageBarrier(_private->vulkan, _private->nativePass.commandBuffer, _private->vulkan.swapchain.currentImage(), 
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	VkClearValue clearValue = { 
		passInfo.target.clearColor.x, 
		passInfo.target.clearColor.y, 
		passInfo.target.clearColor.z, 
		passInfo.target.clearColor.w
	};

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValue;
	renderPassBeginInfo.renderPass = _private->nativePass.renderPass;
	renderPassBeginInfo.renderArea.extent = vulkan.swapchain.extent;
	renderPassBeginInfo.framebuffer = _private->nativePass.framebuffer;
	vkCmdBeginRenderPass(_private->nativePass.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
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
}

void VulkanRenderPass::begin()
{
	VkCommandBuffer cmd = _private->nativePass.commandBuffer;
	
	VkViewport vp[1] = { _private->nativePass.viewport };
	vkCmdSetViewport(cmd, 0, 1, vp);

	VkRect2D sc[1] = { _private->nativePass.scissor };
	vkCmdSetScissor(cmd, 1, 1, sc);
}

void VulkanRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	VulkanPipelineState::Pointer ps = _private->renderer->acquirePipelineState(RenderPass::Pointer(this), 
		batch->material(), batch->vertexStream());

	VkCommandBuffer cmd = _private->nativePass.commandBuffer;
	vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, ps->nativePipeline().pipeline);

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
	vkCmdEndRenderPass(_private->nativePass.commandBuffer);

	vulkan::imageBarrier(_private->vulkan, _private->nativePass.commandBuffer, _private->vulkan.swapchain.currentImage(), 
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	vkEndCommandBuffer(_private->nativePass.commandBuffer);
}

}
