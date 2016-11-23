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
	VulkanPipelineState::Pointer pipeline;
	VulkanTextureSet::Pointer textureSet;
	VulkanBuffer::Pointer vertexBuffer;
	VulkanBuffer::Pointer indexBuffer;
	VkIndexType indexBufferFormat = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;
	uint32_t dynamicOffsets[DescriptorSetClass::DynamicDescriptorsCount]{};
	uint32_t startIndex = InvalidIndex;
	uint32_t indexCount = InvalidIndex;
};

class VulkanRenderPassPrivate : public VulkanNativeRenderPass
{
public:
	VulkanRenderPassPrivate(VulkanState& v, VulkanRenderer* r)
		: vulkan(v), renderer(r)
	{
	}

	VulkanState& vulkan;
	VulkanRenderer* renderer = nullptr;
	VkClearValue clearValues[2]{};
	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

	ConstantBufferEntry variablesData;
	Vector<VulkanRenderBatch> batches;
	Map<uint32_t, VkFramebuffer> framebuffers;

	std::atomic_bool recording{ false };

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

	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VULKAN_CALL(vkCreateSemaphore(vulkan.device, &semaphoreInfo, nullptr, &_private->semaphore));

	const vec4& cl = passInfo.color[0].clearValue;
	_private->clearValues[0].color = { cl.x, cl.y, cl.z, cl.w };
	_private->clearValues[1].depthStencil.depth = passInfo.depth.clearValue.x;

	VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	info.commandPool = vulkan.commandPool;
	info.commandBufferCount = 1;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VULKAN_CALL(vkAllocateCommandBuffers(vulkan.device, &info, &_private->commandBuffer));

	uint32_t colorAttachmentCount = 0;
	Vector<VkAttachmentDescription> attachments;
	Vector<VkAttachmentReference> colorAttachmentReferences;
	attachments.reserve(4);
	colorAttachmentReferences.reserve(4);

	for (const RenderTarget& target : passInfo.color)
	{
		if (target.enabled)
		{
			colorAttachmentReferences.emplace_back();
			VkAttachmentReference& ref = colorAttachmentReferences.back();
			ref.attachment = colorAttachmentCount;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			attachments.emplace_back();
			VkAttachmentDescription& attachment = attachments.back();
			attachment.loadOp = vulkan::frameBufferOperationToLoadOperation(target.loadOperation);
			attachment.storeOp = vulkan::frameBufferOperationToStoreOperation(target.storeOperation);
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			if (target.isDefaultRenderTarget)
			{
				attachment.format = vulkan.swapchain.surfaceFormat.format;
				attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			}
			else
			{
				// TODO : get from texture
				attachment.format = vulkan.swapchain.surfaceFormat.format;
				attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			++colorAttachmentCount;
		}
	}

	if (passInfo.depth.enabled)
	{
		attachments.emplace_back();
		VkAttachmentDescription& attachment = attachments.back();
		attachment.loadOp = vulkan::frameBufferOperationToLoadOperation(passInfo.depth.loadOperation);
		attachment.storeOp = vulkan::frameBufferOperationToStoreOperation(passInfo.depth.storeOperation);
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (passInfo.depth.isDefaultRenderTarget)
		{
			attachment.format = vulkan.swapchain.depthFormat;
			attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			// TODO : get from texture
			attachment.format = vulkan.swapchain.depthFormat;
			attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
	}

	VkAttachmentReference depthAttachmentReference = { colorAttachmentCount, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassInfo = {};
	subpassInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size());
	subpassInfo.pColorAttachments = colorAttachmentReferences.data();
	subpassInfo.pDepthStencilAttachment = passInfo.depth.enabled ? &depthAttachmentReference : nullptr;

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
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

	uint32_t rtWidth = _private->vulkan.swapchain.extent.width;
	uint32_t rtHeight = _private->vulkan.swapchain.extent.height;
	uint32_t currentImageIndex = _private->vulkan.swapchain.currentImageIndex;
	if ((_private->framebufferInfo.width != rtWidth) || (_private->framebufferInfo.height != rtHeight))
	{
		for (auto& fb : _private->framebuffers)
		{
			if (fb.second)
			{
				vkDestroyFramebuffer(_private->vulkan.device, fb.second, nullptr);
			}
			fb.second = nullptr;
		}
	}

	VkFramebuffer currentFramebuffer = _private->framebuffers[currentImageIndex];
	if (currentFramebuffer == nullptr)
	{
		const VulkanSwapchain::RenderTarget& currentRenderTarget = _private->vulkan.swapchain.currentRenderTarget();
		VkImageView attachments[] = { currentRenderTarget.colorView, currentRenderTarget.depthView };
		_private->framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
		_private->framebufferInfo.pAttachments = attachments;
		_private->framebufferInfo.width = rtWidth;
		_private->framebufferInfo.height = rtHeight;
		_private->framebufferInfo.layers = 1;
		_private->framebufferInfo.renderPass = _private->renderPass;

		VULKAN_CALL(vkCreateFramebuffer(_private->vulkan.device, &_private->framebufferInfo, nullptr, &currentFramebuffer));
		_private->framebuffers[currentImageIndex] = currentFramebuffer;
	}

	_private->beginInfo.clearValueCount = sizeof(_private->clearValues) / sizeof(_private->clearValues[0]);
	_private->beginInfo.pClearValues = _private->clearValues;
	_private->beginInfo.renderPass = _private->renderPass;
	_private->beginInfo.renderArea.extent.width = rtWidth;
	_private->beginInfo.renderArea.extent.height = rtHeight;
	_private->beginInfo.framebuffer = currentFramebuffer;

	_private->scissor.extent = _private->beginInfo.renderArea.extent;
	_private->viewport.width = static_cast<float>(rtWidth);
	_private->viewport.height = static_cast<float>(rtHeight);
	_private->viewport.maxDepth = 1.0f;
	_private->loadVariables(info().camera, info().light);

	_private->recording = true;
}

void VulkanRenderPass::pushRenderBatch(const RenderBatch::Pointer& inBatch)
{
	ET_ASSERT(_private->recording);

	_private->batches.emplace_back();
	VulkanRenderBatch& batch = _private->batches.back();

	MaterialInstance::Pointer material = inBatch->material();
	const VulkanProgram::Pointer& program = material->program();

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

	retain();
	batch.pipeline = _private->renderer->acquirePipelineState(VulkanRenderPass::Pointer(this), material, inBatch->vertexStream());
	batch.textureSet = material->textureSet();
	batch.dynamicOffsets[0] = objectVariables.offset();
	batch.dynamicOffsets[1] = material->constantBufferData().offset();
	batch.vertexBuffer = inBatch->vertexStream()->vertexBuffer();
	batch.indexBuffer = inBatch->vertexStream()->indexBuffer();
	batch.indexBufferFormat = vulkan::indexBufferFormat(inBatch->vertexStream()->indexArrayFormat());
	batch.startIndex = inBatch->firstIndex();
	batch.indexCount = inBatch->numIndexes();
	release();
}

void VulkanRenderPass::end()
{
	_private->recording = false;
}

void VulkanRenderPass::recordCommandBuffer()
{
	ET_ASSERT(_private->recording == false);

	_private->renderer->sharedConstantBuffer().flush();
	dynamicConstantBuffer().flush();

	VkCommandBuffer commandBuffer = _private->commandBuffer;

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VULKAN_CALL(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	vkCmdSetScissor(commandBuffer, 0, 1, &_private->scissor);
	vkCmdSetViewport(commandBuffer, 0, 1, &_private->viewport);

	VkPipeline lastPipeline = nullptr;
	VkBuffer lastVertexBuffer = nullptr;
	VkBuffer lastIndexBuffer = nullptr;
	VkIndexType lastIndexType = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;

	VkDescriptorSet descriptorSets[DescriptorSetClass::Count] = {
		_private->dynamicDescriptorSet
	};

	vkCmdBeginRenderPass(commandBuffer, &_private->beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	for (const auto& batch : _private->batches)
	{
		if (batch.pipeline->nativePipeline().pipeline != lastPipeline)
		{
			lastPipeline = batch.pipeline->nativePipeline().pipeline;
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lastPipeline);
		}

		if (batch.vertexBuffer->nativeBuffer().buffer != lastVertexBuffer)
		{
			VkDeviceSize nullOffset = 0;
			lastVertexBuffer = batch.vertexBuffer->nativeBuffer().buffer;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &lastVertexBuffer, &nullOffset);
		}

		if ((batch.indexBuffer->nativeBuffer().buffer != lastIndexBuffer) || (batch.indexBufferFormat != lastIndexType))
		{
			lastIndexBuffer = batch.indexBuffer->nativeBuffer().buffer;
			lastIndexType = batch.indexBufferFormat;
			vkCmdBindIndexBuffer(commandBuffer, lastIndexBuffer, 0, lastIndexType);
		}

		descriptorSets[1] = batch.textureSet->nativeSet().descriptorSet;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			batch.pipeline->nativePipeline().layout, 0,
			DescriptorSetClass::Count, descriptorSets, DescriptorSetClass::DynamicDescriptorsCount, batch.dynamicOffsets);

		vkCmdDrawIndexed(commandBuffer, batch.indexCount, 1, batch.startIndex, 0, 0);
	}
	vkCmdEndRenderPass(commandBuffer);
	VULKAN_CALL(vkEndCommandBuffer(commandBuffer));

	_private->batches.clear();
}

void VulkanRenderPassPrivate::generateDynamicDescriptorSet(RenderPass* pass)
{
	VulkanBuffer::Pointer db = pass->dynamicConstantBuffer().buffer();
	VulkanBuffer::Pointer sb = renderer->sharedConstantBuffer().buffer();
	VkDescriptorBufferInfo passBufferInfo = { db->nativeBuffer().buffer, variablesData.offset(), sizeof(RenderPass::Variables) };
	VkDescriptorBufferInfo objectBufferInfo = { db->nativeBuffer().buffer, 0, VK_WHOLE_SIZE }; // TODO : calculate offset and size
	VkDescriptorBufferInfo materialBufferInfo = { sb->nativeBuffer().buffer, 0, VK_WHOLE_SIZE }; // TODO : calculate offset and size
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
