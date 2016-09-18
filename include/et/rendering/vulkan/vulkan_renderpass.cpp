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
	VulkanRenderer* renderer = nullptr;
};

VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo) 
	: RenderPass(passInfo)
{
	ET_PIMPL_INIT(VulkanRenderPass, vulkan, renderer);

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

	VkAttachmentDescription attachments[2] = { };
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
/*
	VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrierInfo.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrierInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrierInfo.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrierInfo.srcQueueFamilyIndex = vulkan.graphicsQueueIndex;
	barrierInfo.dstQueueFamilyIndex = vulkan.presentQueueIndex;
	barrierInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	barrierInfo.image = _private->vulkan.swapchain.images.at(_private->vulkan.swapchain.currentImageIndex);
	vkCmdPipelineBarrier(_private->nativePass.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
*/
	VkClearValue clearValues[1] = { { 1.0f, 0.5f, 0.25f, 1.0f } };
	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.renderPass = _private->nativePass.renderPass;
	renderPassBeginInfo.renderArea.extent = vulkan.swapchain.extent;
	renderPassBeginInfo.framebuffer = _private->nativePass.framebuffer;
	vkCmdBeginRenderPass(_private->nativePass.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	vkCreateFence(_private->vulkan.device, &fenceInfo, nullptr, &_private->nativePass.submitFence);
}

VulkanRenderPass::~VulkanRenderPass()
{
	vkDestroyFence(_private->vulkan.device, _private->nativePass.submitFence, nullptr);
	vkDestroyFramebuffer(_private->vulkan.device, _private->nativePass.framebuffer, nullptr);
	vkDestroyRenderPass(_private->vulkan.device, _private->nativePass.renderPass, nullptr);
	vkFreeCommandBuffers(_private->vulkan.device, _private->vulkan.commandPool, 1, &_private->nativePass.commandBuffer);
	ET_PIMPL_FINALIZE(VulkanRenderPass)
}

const VulkanNativeRenderPass& VulkanRenderPass::nativeRenderPass() const
{
	return _private->nativePass;
}

void VulkanRenderPass::endRenderPass()
{
	vkCmdEndRenderPass(_private->nativePass.commandBuffer);
/*
	VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrierInfo.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrierInfo.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrierInfo.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrierInfo.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	
	barrierInfo.srcQueueFamilyIndex = _private->vulkan.presentQueueIndex;
	barrierInfo.dstQueueFamilyIndex = _private->vulkan.graphicsQueueIndex;
	barrierInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	barrierInfo.image = _private->vulkan.swapchain.images.at(_private->vulkan.swapchain.currentImageIndex);
	vkCmdPipelineBarrier(_private->nativePass.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
*/
	vkEndCommandBuffer(_private->nativePass.commandBuffer);
}

void VulkanRenderPass::submit()
{
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_private->nativePass.commandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_private->vulkan.semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &_private->vulkan.semaphores.renderComplete;
	submitInfo.pWaitDstStageMask = waitStages;

	VULKAN_CALL(vkQueueSubmit(_private->vulkan.queue, 1, &submitInfo, _private->nativePass.submitFence));
	VULKAN_CALL(vkWaitForFences(_private->vulkan.device, 1, &_private->nativePass.submitFence, VK_TRUE, UINT64_MAX));
}

void VulkanRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	return;

	VulkanPipelineState::Pointer ps = _private->renderer->createPipelineState(RenderPass::Pointer(this), batch->material(), batch->vertexStream());
	VulkanIndexBuffer::Pointer ib = batch->vertexStream()->indexBuffer();
	VulkanVertexBuffer::Pointer vb = batch->vertexStream()->vertexBuffer();

	VkDeviceSize vbOffsets[1] = { 0 };
	VkBuffer vBuffers[1] = { vb->nativeBuffer().buffer() };
	VkCommandBuffer cmd = _private->nativePass.commandBuffer;
	VkViewport vp[1] = { _private->nativePass.viewport };
	vkCmdSetViewport(cmd, 0, 1, vp);
	vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, ps->nativePipeline().pipeline);
	vkCmdBindIndexBuffer(cmd, ib->nativeBuffer().buffer(), 0, VkIndexType::VK_INDEX_TYPE_UINT32);
	vkCmdBindVertexBuffers(cmd, 0, 1, vBuffers, vbOffsets);
	vkCmdDrawIndexed(cmd, batch->numIndexes(), 1, batch->firstIndex(), 0, 0);
}

}
