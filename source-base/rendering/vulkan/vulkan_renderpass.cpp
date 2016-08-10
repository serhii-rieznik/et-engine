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
	VulkanRenderPassPrivate(VulkanState& v) 
		: vulkan(v) { }

	VulkanState& vulkan;
	VkFramebuffer framebuffer = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	VkRenderPass renderPass = nullptr;
	VkFence submitFence = nullptr;
};

VulkanRenderPass::VulkanRenderPass(VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo) 
	: RenderPass(passInfo)
{
	ET_PIMPL_INIT(VulkanRenderPass, vulkan);

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
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

	VULKAN_CALL(vkCreateRenderPass(vulkan.device, &createInfo, nullptr, &_private->renderPass));

	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &_private->vulkan.swapchain.imageViews.at(_private->vulkan.swapchain.currentImageIndex);
	framebufferInfo.width = _private->vulkan.swapchain.extent.width;
	framebufferInfo.height = _private->vulkan.swapchain.extent.height;
	framebufferInfo.layers = 1;
	framebufferInfo.renderPass = _private->renderPass;
	VULKAN_CALL(vkCreateFramebuffer(vulkan.device, &framebufferInfo, nullptr, &_private->framebuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(_private->commandBuffer, &commandBufferBeginInfo);

	VkClearValue clearValues[1] = { { 1.0f, 0.5f, 0.25f, 1.0f } };

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.renderPass = _private->renderPass;
	renderPassBeginInfo.renderArea.extent = vulkan.swapchain.extent;
	renderPassBeginInfo.framebuffer = _private->framebuffer;
	vkCmdBeginRenderPass(_private->commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	vkCreateFence(_private->vulkan.device, &fenceInfo, nullptr, &_private->submitFence);
}

VulkanRenderPass::~VulkanRenderPass()
{
	vkDestroyFence(_private->vulkan.device, _private->submitFence, nullptr);
	vkDestroyFramebuffer(_private->vulkan.device, _private->framebuffer, nullptr);
	vkDestroyRenderPass(_private->vulkan.device, _private->renderPass, nullptr);
	vkFreeCommandBuffers(_private->vulkan.device, _private->vulkan.commandPool, 1, &_private->commandBuffer);
	ET_PIMPL_FINALIZE(VulkanRenderPass)
}

void VulkanRenderPass::endRenderPass()
{
	vkCmdEndRenderPass(_private->commandBuffer);
	vkEndCommandBuffer(_private->commandBuffer);
}

void VulkanRenderPass::submit()
{
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_private->commandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_private->vulkan.semaphores.imageAvailable;
	submitInfo.pWaitDstStageMask = waitStages;

	VULKAN_CALL(vkQueueSubmit(_private->vulkan.queue, 1, &submitInfo, _private->submitFence));
	VULKAN_CALL(vkWaitForFences(_private->vulkan.device, 1, &_private->submitFence, VK_TRUE, UINT64_MAX));
}

void VulkanRenderPass::pushRenderBatch(RenderBatch::Pointer)
{

}

}
