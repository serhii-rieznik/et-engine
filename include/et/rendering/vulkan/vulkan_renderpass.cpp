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

struct VulkanCommand
{
	enum class Type : uint32_t
	{
		Undefined,
		BeginRenderPass,
		RenderBatch,
		Barrier,
		NextRenderPass,
		EndRenderPass,
	} type = Type::Undefined;

	VulkanRenderBatch batch;
	RenderSubpass subpass;

	VulkanCommand(Type t) :
		type(t) { }

	VulkanCommand(VulkanRenderBatch&& b) :
		type(Type::RenderBatch), batch(b) { }

	VulkanCommand(const RenderSubpass& sp) :
		type(Type::BeginRenderPass), subpass(sp) { }
};

struct VulkanRenderSubpass
{
	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	VkViewport viewport { };
	VkRect2D scissor { };
};

uint64_t framebufferHash(uint32_t imageIndex, uint32_t mipLevel, uint32_t layer)
{
	return static_cast<uint64_t>(layer) | 
		((static_cast<uint64_t>(mipLevel) & 0xFFFF) << 32) | 
		((static_cast<uint64_t>(imageIndex) & 0xFFFF) << 48);
}

class VulkanRenderPassPrivate : public VulkanNativeRenderPass
{
public:
	VulkanRenderPassPrivate(VulkanState& v, VulkanRenderer* r)
		: vulkan(v), renderer(r) { }

	VulkanState& vulkan;
	VulkanRenderer* renderer = nullptr;

	ConstantBufferEntry variablesData;
	Vector<VulkanCommand> commands;
	Vector<VkClearValue> clearValues;
	Map<uint64_t, VulkanRenderSubpass> subpasses;
	Vector<VulkanRenderSubpass> subpassSequence;
	uint32_t currentImageIndex = 0;
	uint32_t currentSubpassIndex = 0;

	std::atomic_bool recording{ false };

	void generateDynamicDescriptorSet(RenderPass* pass);
	void loadVariables(Camera::Pointer camera, Light::Pointer light);
};

VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo)
	: RenderPass(renderer, passInfo)
{
	ET_PIMPL_INIT(VulkanRenderPass, vulkan, renderer);

	ET_ASSERT(!passInfo.name.empty());

	_private->variablesData = dynamicConstantBuffer().staticAllocate(sizeof(Variables));
	_private->commands.reserve(256);
	_private->generateDynamicDescriptorSet(this);
	_private->subpassSequence.reserve(64);

	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VULKAN_CALL(vkCreateSemaphore(vulkan.device, &semaphoreInfo, nullptr, &_private->semaphore));

	VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	info.commandPool = vulkan.commandPool;
	info.commandBufferCount = 1;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VULKAN_CALL(vkAllocateCommandBuffers(vulkan.device, &info, &_private->commandBuffer));

	Vector<VkAttachmentDescription> attachments;
	attachments.reserve(MaxRenderTargets + 1);
	Vector<VkAttachmentReference> colorAttachmentReferences;
	colorAttachmentReferences.reserve(MaxRenderTargets + 1);

	uint32_t colorAttachmentIndex = 0;
	VkAttachmentReference depthAttachmentReference = { };
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
		attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		if (passInfo.depth.useDefaultRenderTarget)
		{
			attachment.format = vulkan.swapchain.depthFormat;
		}
		else
		{
			ET_ASSERT(passInfo.depth.texture.valid());
			VulkanTexture::Pointer texture = passInfo.depth.texture;
			attachment.format = texture->nativeTexture().format;
		}
		if (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
		{
			_private->clearValues.emplace_back();
			_private->clearValues.back().depthStencil = { passInfo.depth.clearValue.x };
		}
		depthAttachmentReference.layout = attachment.finalLayout;
		++colorAttachmentIndex;
	}

	for (const RenderTarget& target : passInfo.color)
	{
		if (target.enabled)
		{
			attachments.emplace_back();
			VkAttachmentDescription& attachment = attachments.back();
			attachment.loadOp = vulkan::frameBufferOperationToLoadOperation(target.loadOperation);
			attachment.storeOp = vulkan::frameBufferOperationToStoreOperation(target.storeOperation);
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			if (target.useDefaultRenderTarget)
			{
				attachment.format = vulkan.swapchain.surfaceFormat.format;
			}
			else
			{
				ET_ASSERT(target.texture.valid());
				const VulkanTexture::Pointer& texture = target.texture;
				attachment.format = texture->nativeTexture().format;
			}
			if (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
			{
				const vec4& cl = target.clearValue;
				_private->clearValues.emplace_back();
				_private->clearValues.back().color = { cl.x, cl.y, cl.z, cl.w };
			}
			colorAttachmentReferences.push_back({ colorAttachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			++colorAttachmentIndex;
		}
		else
		{
			break;
		}
	}

	VkSubpassDescription subpassInfo = { };
	subpassInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size());
	subpassInfo.pColorAttachments = colorAttachmentReferences.empty() ? nullptr : colorAttachmentReferences.data();
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
	for (const auto& fb : _private->subpasses)
		vkDestroyFramebuffer(_private->vulkan.device, fb.second.beginInfo.framebuffer, nullptr);

	vkDestroyRenderPass(_private->vulkan.device, _private->renderPass, nullptr);
	vkFreeCommandBuffers(_private->vulkan.device, _private->vulkan.commandPool, 1, &_private->commandBuffer);

	vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptorPool, 1, &_private->dynamicDescriptorSet);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->dynamicDescriptorSetLayout, nullptr);

	dynamicConstantBuffer().free(_private->variablesData);

	ET_PIMPL_FINALIZE(VulkanRenderPass);
}

const VulkanNativeRenderPass& VulkanRenderPass::nativeRenderPass() const
{
	return *(_private);
}

void VulkanRenderPass::begin(const RenderPassBeginInfo& beginInfo)
{
	ET_ASSERT(_private->recording == false);

	_private->subpassSequence.clear();

	Texture::Pointer renderTarget = info().color[0].texture;
	Texture::Pointer depthTarget = info().depth.texture;
	bool useCustomColor = info().color[0].enabled && !info().color[0].useDefaultRenderTarget;
	bool useCustomDepth = info().depth.enabled && !info().depth.useDefaultRenderTarget;
	uint32_t defaultWidth = _private->vulkan.swapchain.extent.width;
	uint32_t defaultHeight = _private->vulkan.swapchain.extent.height;
	_private->currentImageIndex = _private->vulkan.swapchain.currentImageIndex;

	if ((useCustomColor && renderTarget.valid()) || (useCustomDepth && depthTarget.valid()))
		_private->currentImageIndex = 0;

	for (const RenderSubpass& subpass : beginInfo.subpasses)
	{
		if (subpass.level > 0)
		{
			ET_ASSERT(useCustomColor);
			ET_ASSERT(useCustomDepth);
		}
		uint64_t hash = framebufferHash(_private->currentImageIndex, subpass.level, subpass.layer);

		uint32_t width = defaultWidth;
		uint32_t height = defaultHeight;
		if (useCustomColor)
		{
			vec2i sz = renderTarget->size(subpass.level);
			width = static_cast<uint32_t>(sz.x);
			height = static_cast<uint32_t>(sz.y);
		}

		VulkanRenderSubpass& subpassInfo = _private->subpasses[hash];
		if ((subpassInfo.beginInfo.framebuffer != nullptr) &&
			((subpassInfo.framebufferInfo.width != width) || (subpassInfo.framebufferInfo.height != height)))
		{
			vkDestroyFramebuffer(_private->vulkan.device, subpassInfo.beginInfo.framebuffer, nullptr);
			subpassInfo.beginInfo.framebuffer = nullptr;
		}

		if (subpassInfo.beginInfo.framebuffer == nullptr)
		{
			Vector<VkImageView> attachments;
			attachments.reserve(MaxRenderTargets + 1);

			if (info().depth.enabled && info().depth.useDefaultRenderTarget)
			{
				attachments.emplace_back(_private->vulkan.swapchain.depthBuffer.depthView);
			}
			else if (info().depth.enabled)
			{
				ET_ASSERT(info().depth.texture.valid());
				VulkanTexture::Pointer texture = info().depth.texture;
				attachments.emplace_back(texture->nativeTexture().completeImageView);
			}

			for (const RenderTarget& rt : info().color)
			{
				if (rt.enabled && rt.useDefaultRenderTarget)
				{
					attachments.emplace_back(_private->vulkan.swapchain.currentRenderTarget().colorView);
				}
				else if (rt.enabled)
				{
					ET_ASSERT(rt.texture.valid());
					const VulkanTexture::Pointer& texture = rt.texture;
					attachments.emplace_back(texture->nativeTexture().layerImageView(subpass.layer));
				}
				else
				{
					break;
				}
			}
			subpassInfo.framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			subpassInfo.framebufferInfo.pAttachments = attachments.data();
			subpassInfo.framebufferInfo.width = width;
			subpassInfo.framebufferInfo.height = height;
			subpassInfo.framebufferInfo.layers = 1;
			subpassInfo.framebufferInfo.renderPass = _private->renderPass;
			VULKAN_CALL(vkCreateFramebuffer(_private->vulkan.device, &subpassInfo.framebufferInfo, nullptr, &subpassInfo.beginInfo.framebuffer));
		}

		subpassInfo.beginInfo.clearValueCount = static_cast<uint32_t>(_private->clearValues.size());
		subpassInfo.beginInfo.pClearValues = _private->clearValues.data();
		subpassInfo.beginInfo.renderPass = _private->renderPass;
		subpassInfo.beginInfo.renderArea.extent.width = width;
		subpassInfo.beginInfo.renderArea.extent.height = height;

		subpassInfo.viewport.width = static_cast<float>(width);
		subpassInfo.viewport.height = static_cast<float>(height);
		subpassInfo.viewport.maxDepth = 1.0f;

		subpassInfo.scissor.extent.width = width;
		subpassInfo.scissor.extent.height = height;
		
		_private->subpassSequence.emplace_back(subpassInfo);
	}
	
	_private->loadVariables(info().camera, info().light);

	_private->recording = true;
	_private->currentSubpassIndex = 0;
	_private->commands.emplace_back(beginInfo.subpasses.front());
}

void VulkanRenderPass::nextSubpass()
{
	ET_ASSERT(_private->recording);

	if (_private->currentSubpassIndex + 1 >= _private->subpassSequence.size())
		return;

	_private->commands.emplace_back(VulkanCommand::Type::EndRenderPass);
	_private->commands.emplace_back(VulkanCommand::Type::NextRenderPass);
	_private->commands.emplace_back(VulkanCommand::Type::BeginRenderPass);
	++_private->currentSubpassIndex;
}

void VulkanRenderPass::pushRenderBatch(const RenderBatch::Pointer& inBatch)
{
	ET_ASSERT(_private->recording);

	retain();
	MaterialInstance::Pointer material = inBatch->material();
	VulkanProgram::Pointer program = inBatch->material()->configuration(info().name).program;
	VulkanPipelineState::Pointer pipelineState = _private->renderer->acquirePipelineState(VulkanRenderPass::Pointer(this), material, inBatch->vertexStream());
	release();

	if (pipelineState->nativePipeline().pipeline == nullptr)
		return;

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

	for (const auto& sh : sharedTextures())
	{
		material->setTexture(sh.first, sh.second.first);
		material->setSampler(sh.first, sh.second.second);
	}

	VulkanRenderBatch batch;
	batch.textureSet = material->textureSet(info().name);
	batch.dynamicOffsets[0] = objectVariables.offset();
	batch.dynamicOffsets[1] = material->constantBufferData(info().name).offset();
	batch.vertexBuffer = inBatch->vertexStream()->vertexBuffer();
	batch.indexBuffer = inBatch->vertexStream()->indexBuffer();
	batch.indexBufferFormat = vulkan::indexBufferFormat(inBatch->vertexStream()->indexArrayFormat());
	batch.startIndex = inBatch->firstIndex();
	batch.indexCount = inBatch->numIndexes();
	batch.pipeline = pipelineState;

	_private->commands.emplace_back(std::move(batch));
}

void VulkanRenderPass::end()
{
	ET_ASSERT(_private->recording);

	_private->recording = false;
	_private->commands.emplace_back(VulkanCommand::Type::EndRenderPass);
}

void VulkanRenderPass::recordCommandBuffer()
{
	ET_ASSERT(_private->recording == false);

	_private->renderer->sharedConstantBuffer().flush();
	dynamicConstantBuffer().flush();

	VkCommandBuffer commandBuffer = _private->commandBuffer;
	VkPipeline lastPipeline = nullptr;
	VkBuffer lastVertexBuffer = nullptr;
	VkBuffer lastIndexBuffer = nullptr;
	VkIndexType lastIndexType = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;

	VkDescriptorSet descriptorSets[DescriptorSetClass::Count] = {
		_private->dynamicDescriptorSet,
		nullptr,
	};

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VULKAN_CALL(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	uint32_t subpassIndex = 0;
	for (const VulkanCommand& cmd : _private->commands)
	{
		if (cmd.type == VulkanCommand::Type::BeginRenderPass)
		{
			const VulkanRenderSubpass& subpass = _private->subpassSequence.at(subpassIndex);
			vkCmdBeginRenderPass(commandBuffer, &subpass.beginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetScissor(commandBuffer, 0, 1, &subpass.scissor);
			vkCmdSetViewport(commandBuffer, 0, 1, &subpass.viewport);
		}
		else if (cmd.type == VulkanCommand::Type::RenderBatch)
		{
			const VulkanRenderBatch& batch = cmd.batch;
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

			descriptorSets[DescriptorSetClass::Textures] = batch.textureSet->nativeSet().descriptorSet;

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.pipeline->nativePipeline().layout, 0,
				DescriptorSetClass::Count, descriptorSets, DescriptorSetClass::DynamicDescriptorsCount, batch.dynamicOffsets);

			vkCmdDrawIndexed(commandBuffer, batch.indexCount, 1, batch.startIndex, 0, 0);
		}
		else if (cmd.type == VulkanCommand::Type::EndRenderPass)
		{
			vkCmdEndRenderPass(commandBuffer);
		}
		else if (cmd.type == VulkanCommand::Type::NextRenderPass)
		{
			++subpassIndex;
		}
		else
		{
			ET_FAIL("Not implemented");
		}
	}
	
	VULKAN_CALL(vkEndCommandBuffer(commandBuffer));
	_private->commands.clear();
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
	descriptorAllocInfo.descriptorPool = vulkan.descriptorPool;
	descriptorAllocInfo.descriptorSetCount = 1;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &descriptorAllocInfo, &dynamicDescriptorSet));

	for (VkWriteDescriptorSet& wd : writeSets)
		wd.dstSet = dynamicDescriptorSet;

	uint32_t writeSetsCount = static_cast<uint32_t>(sizeof(writeSets) / sizeof(writeSets[0]));
	vkUpdateDescriptorSets(vulkan.device, writeSetsCount, writeSets, 0, nullptr);
}

void VulkanRenderPassPrivate::loadVariables(Camera::Pointer camera, Light::Pointer light)
{
	RenderPass::Variables* vptr = reinterpret_cast<RenderPass::Variables*>(variablesData.data());
	if (camera.valid())
	{
		vptr->viewProjection = camera->viewProjectionMatrix();
		vptr->projection = camera->projectionMatrix();
		vptr->view = camera->viewMatrix();
		vptr->inverseViewProjection = camera->inverseViewProjectionMatrix();
		vptr->inverseProjection = camera->inverseProjectionMatrix();
		vptr->inverseView = camera->inverseViewMatrix();
		vptr->cameraPosition = vec4(camera->position());
		vptr->cameraDirection = vec4(camera->direction());
		vptr->cameraUp = vec4(camera->up());
	}

	if (light.valid())
	{
		vptr->lightPosition = light->type() == Light::Type::Directional ? vec4(-light->direction(), 0.0f) : vec4(light->position());
		vptr->lightProjection = light->viewProjectionMatrix() * lightProjectionMatrix;
	}
}

}
