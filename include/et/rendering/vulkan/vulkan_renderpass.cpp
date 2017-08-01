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
	VulkanTextureSet::Pointer imageSet;
	VulkanBuffer::Pointer vertexBuffer;
	VulkanBuffer::Pointer indexBuffer;
	ConstantBufferEntry::Pointer dynamicOffsets[DescriptorSetClass::DynamicDescriptorsCount];
	VkIndexType indexBufferFormat = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;
	uint32_t startIndex = InvalidIndex;
	uint32_t indexCount = InvalidIndex;
};

struct VulkanComputeStruct
{
	VulkanTextureSet::Pointer textureSet;
	VulkanTextureSet::Pointer imageSet;
	VulkanCompute::Pointer compute;
	ConstantBufferEntry::Pointer dynamicOffsets[DescriptorSetClass::DynamicDescriptorsCount];
	vec3i dimension = vec3i(0);
};

struct VulkanCommand
{
	enum class Type : uint32_t
	{
		Undefined,
		BeginRenderPass,
		RenderBatch,
		ImageBarrier,
		CopyImage,
		DispatchCompute,
		NextRenderPass,
		EndRenderPass,
		Debug,
	} type = Type::Undefined;

	VulkanRenderBatch batch;
	VulkanTexture::Pointer sourceImage;
	ResourceBarrier imageBarrier;
	RenderSubpass subpass;
	
	VulkanTexture::Pointer destImage;
	CopyDescriptor copyDescriptor;

	VulkanComputeStruct compute;

	VulkanCommand(Type t) :
		type(t) { }

	VulkanCommand(VulkanRenderBatch&& b) :
		type(Type::RenderBatch), batch(b) { }

	VulkanCommand(VulkanComputeStruct& c) :
		type(Type::DispatchCompute), compute(c) { }

	VulkanCommand(const RenderSubpass& sp) :
		type(Type::BeginRenderPass), subpass(sp) { }

	VulkanCommand(const Texture::Pointer& tex, const ResourceBarrier& b) :
		type(Type::ImageBarrier), sourceImage(tex), imageBarrier(b) { }

	VulkanCommand(const Texture::Pointer& texFrom, const Texture::Pointer& texTo, const CopyDescriptor& desc) :
		type(Type::CopyImage), sourceImage(texFrom), destImage(texTo), copyDescriptor(desc) { }

	~VulkanCommand() { }
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

	std::array<Vector<VulkanCommand>, RendererFrameCount> commands;
	Vector<VkClearValue> clearValues;
	Map<uint64_t, VulkanRenderSubpass> subpasses;
	Vector<VulkanRenderSubpass> subpassSequence;
	uint32_t currentSubpassIndex = InvalidIndex;
	uint32_t frameIndex = 0;

	std::atomic_bool recording{ false };

	void generateDynamicDescriptorSet(RenderPass* pass);
};

VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo)
	: RenderPass(renderer, passInfo)
{
	ET_PIMPL_INIT(VulkanRenderPass, vulkan, renderer);

	ET_ASSERT(!passInfo.name.empty());

	_private->commands[0].reserve(256);
	_private->commands[1].reserve(256);
	_private->commands[2].reserve(256);
	_private->generateDynamicDescriptorSet(this);
	_private->subpassSequence.reserve(64);

	for (uint32_t i = 0; i < RendererFrameCount; ++i)
	{
		VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VULKAN_CALL(vkCreateSemaphore(vulkan.device, &semaphoreInfo, nullptr, &_private->content[i].semaphore));

		VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		info.commandPool = vulkan.graphicsCommandPool;
		info.commandBufferCount = 1;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VULKAN_CALL(vkAllocateCommandBuffers(vulkan.device, &info, &_private->content[i].commandBuffer));
	}

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
	for (uint32_t i = 0; i < RendererFrameCount; ++i)
	{
		vkFreeCommandBuffers(_private->vulkan.device, _private->vulkan.graphicsCommandPool, 1, &_private->content[i].commandBuffer);
		vkDestroySemaphore(_private->vulkan.device, _private->content[i].semaphore, nullptr);
	}
	vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptorPool, 1, &_private->dynamicDescriptorSet);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->dynamicDescriptorSetLayout, nullptr);
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
	_private->frameIndex = _private->renderer->frameIndex();

	Texture::Pointer renderTarget = info().color[0].texture;
	Texture::Pointer depthTarget = info().depth.texture;
	bool useCustomColor = info().color[0].enabled && !info().color[0].useDefaultRenderTarget;
	bool useCustomDepth = info().depth.enabled && !info().depth.useDefaultRenderTarget;
	uint32_t defaultWidth = _private->vulkan.swapchain.extent.width;
	uint32_t defaultHeight = _private->vulkan.swapchain.extent.height;
	uint32_t framebufferIndex = _private->vulkan.swapchain.swapchainImageIndex;

	if ((useCustomColor && renderTarget.valid()) || (useCustomDepth && depthTarget.valid()))
		framebufferIndex = 0;

	for (const RenderSubpass& subpass : beginInfo.subpasses)
	{
		uint64_t hash = framebufferHash(framebufferIndex, subpass.level, subpass.layer);

		uint32_t width = defaultWidth;
		uint32_t height = defaultHeight;
		if (useCustomColor)
		{
			vec2i sz = renderTarget->size(subpass.level);
			width = static_cast<uint32_t>(sz.x);
			height = static_cast<uint32_t>(sz.y);
		}
		else if (useCustomDepth)
		{
			vec2i sz = depthTarget->size(subpass.level);
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
				attachments.emplace_back(texture->nativeTexture().imageView(subpass.layer, subpass.level));
			}

			for (const RenderTarget& rt : info().color)
			{
				if (rt.enabled && rt.useDefaultRenderTarget)
				{
					attachments.emplace_back(_private->vulkan.swapchain.currentFrame().colorView);
				}
				else if (rt.enabled)
				{
					ET_ASSERT(rt.texture.valid());
					VulkanTexture::Pointer texture = rt.texture;
					attachments.emplace_back(texture->nativeTexture().imageView(subpass.layer, subpass.level));
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
	
	_private->recording = true;
	_private->currentSubpassIndex = InvalidIndex;
	_private->commands[_private->frameIndex].clear();

	setSharedVariable(ObjectVariable::DeltaTime, application().mainRunLoop().lastFrameTime());
	setSharedVariable(ObjectVariable::ContinuousTime, application().mainRunLoop().time());
	/*
	vec4 viewport(_private->subpassSequence.front().viewport.x, _private->subpassSequence.front().viewport.y,
		_private->subpassSequence.front().viewport.width, _private->subpassSequence.front().viewport.height);
	setSharedVariable(ObjectVariable::Viewport, viewport);
	*/
}

void VulkanRenderPass::endSubpass()
{
	ET_ASSERT(_private->recording);

	_private->commands[_private->frameIndex].emplace_back(VulkanCommand::Type::EndRenderPass);
}

void VulkanRenderPass::nextSubpass()
{
	ET_ASSERT(_private->recording);

	uint32_t nextSubpassIndex = (_private->currentSubpassIndex == InvalidIndex) ? 0 : _private->currentSubpassIndex + 1;
	
	if (nextSubpassIndex < _private->subpassSequence.size())
	{
		_private->commands[_private->frameIndex].emplace_back(VulkanCommand::Type::NextRenderPass);
		_private->commands[_private->frameIndex].emplace_back(VulkanCommand::Type::BeginRenderPass);
		_private->currentSubpassIndex = nextSubpassIndex;

		vec4 viewport(
			_private->subpassSequence[_private->currentSubpassIndex].viewport.x,
			_private->subpassSequence[_private->currentSubpassIndex].viewport.y,
			_private->subpassSequence[_private->currentSubpassIndex].viewport.width,
			_private->subpassSequence[_private->currentSubpassIndex].viewport.height);
		setSharedVariable(ObjectVariable::Viewport, viewport);
	}
}

void VulkanRenderPass::pushRenderBatch(const RenderBatch::Pointer& inBatch)
{
	ET_ASSERT(_private->recording);

	retain();
	MaterialInstance::Pointer material = inBatch->material();
	VulkanProgram::Pointer program = inBatch->material()->configuration(info().name).program;
	VulkanPipelineState::Pointer pipelineState = _private->renderer->acquireGraphicsPipeline(VulkanRenderPass::Pointer(this), material, inBatch->vertexStream());
	release();

	if (pipelineState->nativePipeline().pipeline == nullptr)
		return;

	ConstantBufferEntry::Pointer objectVariables;
	if (program->reflection().objectVariablesBufferSize > 0)
	{
		objectVariables = _private->renderer->sharedConstantBuffer().allocate(
			program->reflection().objectVariablesBufferSize, ConstantBufferDynamicAllocation);

		for (const auto& v : sharedVariables())
		{
			const Program::Variable& var = program->reflection().objectVariables[v.first];
			
			if (v.second.isSet() && var.enabled)
				memcpy(objectVariables->data() + var.offset, v.second.data, v.second.size);
		}
	}

	for (const auto& sh : sharedTextures())
	{
		material->setTexture(sh.first, sh.second.first);
		material->setSampler(sh.first, sh.second.second);
	}

	_private->commands[_private->frameIndex].emplace_back(VulkanCommand::Type::RenderBatch);

	VulkanRenderBatch& batch = _private->commands[_private->frameIndex].back().batch;
	batch.textureSet = material->textureSet(info().name);
	batch.imageSet = material->imageSet(info().name);
	batch.dynamicOffsets[0] = objectVariables;
	batch.dynamicOffsets[1] = material->constantBufferData(info().name);
	batch.vertexBuffer = inBatch->vertexStream()->vertexBuffer();
	batch.indexBuffer = inBatch->vertexStream()->indexBuffer();
	batch.indexBufferFormat = vulkan::indexBufferFormat(inBatch->vertexStream()->indexArrayFormat());
	batch.startIndex = inBatch->firstIndex();
	batch.indexCount = inBatch->numIndexes();
	batch.pipeline = pipelineState;
}

void VulkanRenderPass::pushImageBarrier(const Texture::Pointer& tex, const ResourceBarrier& barrier)
{
	_private->commands[_private->frameIndex].emplace_back(tex, barrier);
}

void VulkanRenderPass::copyImage(const Texture::Pointer& tFrom, const Texture::Pointer& tTo, const CopyDescriptor& desc)
{
	_private->commands[_private->frameIndex].emplace_back(tFrom, tTo, desc);
}

void VulkanRenderPass::end()
{
	ET_ASSERT(_private->recording);
	
	_private->recording = false;
}

void VulkanRenderPass::debug()
{
	_private->commands[_private->frameIndex].emplace_back(VulkanCommand::Type::Debug);
}

void VulkanRenderPass::recordCommandBuffer()
{
	ET_ASSERT(_private->recording == false);

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
	VkPipeline lastPipeline = nullptr;
	VkBuffer lastVertexBuffer = nullptr;
	VkBuffer lastIndexBuffer = nullptr;
	VkIndexType lastIndexType = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;

	uint32_t dynamicOffsets[DescriptorSetClass::DynamicDescriptorsCount] = { };

	VkDescriptorSet descriptorSets[DescriptorSetClass_Count] = {
		_private->dynamicDescriptorSet,
		nullptr,
		nullptr,
	};

	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VULKAN_CALL(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	bool renderPassStarted = false;
	uint32_t subframeIndex = InvalidIndex;
	for (const VulkanCommand& cmd : _private->commands[_private->frameIndex])
	{
		switch (cmd.type)
		{
		case VulkanCommand::Type::BeginRenderPass:
		{
			ET_ASSERT(renderPassStarted == false);

			renderPassStarted = true;
			const VulkanRenderSubpass& subpass = _private->subpassSequence.at(subframeIndex);
			vkCmdBeginRenderPass(commandBuffer, &subpass.beginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetScissor(commandBuffer, 0, 1, &subpass.scissor);
			vkCmdSetViewport(commandBuffer, 0, 1, &subpass.viewport);
			break;
		}

		case VulkanCommand::Type::EndRenderPass:
		{
			ET_ASSERT(renderPassStarted == true);
			vkCmdEndRenderPass(commandBuffer);
			renderPassStarted = false;
			break;
		}

		case VulkanCommand::Type::NextRenderPass:
		{
			subframeIndex = (subframeIndex == InvalidIndex) ? 0 : (subframeIndex + 1);
			break;
		}

		case VulkanCommand::Type::RenderBatch:
		{
			ET_ASSERT(renderPassStarted);

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
			descriptorSets[DescriptorSetClass::Images] = batch.imageSet->nativeSet().descriptorSet;

			for (uint32_t i = 0, e = DescriptorSetClass::DynamicDescriptorsCount; i < e; ++i)
			{
				if (batch.dynamicOffsets[i].valid())
					dynamicOffsets[i] = batch.dynamicOffsets[i]->offset();
			}

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.pipeline->nativePipeline().layout, 0,
				DescriptorSetClass_Count, descriptorSets, DescriptorSetClass::DynamicDescriptorsCount, dynamicOffsets);

			vkCmdDrawIndexed(commandBuffer, batch.indexCount, 1, batch.startIndex, 0, 0);
			break;
		}
		
		case VulkanCommand::Type::ImageBarrier:
		{
			VulkanTexture::Pointer tex = cmd.sourceImage;
			VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrier.image = tex->nativeTexture().image;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = vulkan::texureStateToAccessFlags(cmd.imageBarrier.toState);
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = vulkan::texureStateToImageLayout(cmd.imageBarrier.toState);
			barrier.srcQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
			barrier.dstQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;

			barrier.subresourceRange = { tex->nativeTexture().aspect, cmd.imageBarrier.firstLevel,
				cmd.imageBarrier.levelCount, cmd.imageBarrier.firstLayer, cmd.imageBarrier.layerCount };

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			break;
		}
		
		case VulkanCommand::Type::CopyImage:
		{
			VulkanTexture::Pointer tFrom = cmd.sourceImage;
			VulkanTexture::Pointer tTo = cmd.destImage;
			VkImageCopy region = { };
			region.srcSubresource.aspectMask = tFrom->nativeTexture().aspect;
			region.srcSubresource.baseArrayLayer = cmd.copyDescriptor.layerFrom;
			region.srcSubresource.layerCount = 1;
			region.srcSubresource.mipLevel = cmd.copyDescriptor.levelFrom;
			region.srcOffset.x = cmd.copyDescriptor.offsetFrom.x;
			region.srcOffset.y = cmd.copyDescriptor.offsetFrom.y;
			region.srcOffset.z = cmd.copyDescriptor.offsetFrom.z;
			region.dstSubresource.aspectMask = tTo->nativeTexture().aspect;
			region.dstSubresource.baseArrayLayer = cmd.copyDescriptor.layerTo;
			region.dstSubresource.layerCount = 1;
			region.dstSubresource.mipLevel = cmd.copyDescriptor.levelTo;
			region.dstOffset.x = cmd.copyDescriptor.offsetTo.x;
			region.dstOffset.y = cmd.copyDescriptor.offsetTo.y;
			region.dstOffset.z = cmd.copyDescriptor.offsetTo.z;
			region.extent.width = cmd.copyDescriptor.size.x;
			region.extent.height = cmd.copyDescriptor.size.y;
			region.extent.depth = cmd.copyDescriptor.size.z;
			vkCmdCopyImage(commandBuffer, tFrom->nativeTexture().image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				tTo->nativeTexture().image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			break;
		}
		
		case VulkanCommand::Type::DispatchCompute:
		{
			if (cmd.compute.compute->nativeCompute().pipeline != lastPipeline)
			{
				lastPipeline = cmd.compute.compute->nativeCompute().pipeline;
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, lastPipeline);
			}

			descriptorSets[DescriptorSetClass::Textures] = cmd.compute.textureSet->nativeSet().descriptorSet;
			descriptorSets[DescriptorSetClass::Images] = cmd.compute.imageSet->nativeSet().descriptorSet;

			for (uint32_t i = 0, e = DescriptorSetClass::DynamicDescriptorsCount; i < e; ++i)
			{
				if (cmd.compute.dynamicOffsets[i].valid())
					dynamicOffsets[i] = cmd.compute.dynamicOffsets[i]->offset();
			}

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cmd.compute.compute->nativeCompute().layout,
				0, DescriptorSetClass_Count, descriptorSets, DescriptorSetClass::DynamicDescriptorsCount, dynamicOffsets);

			uint32_t tx = static_cast<uint32_t>(cmd.compute.dimension.x);
			uint32_t ty = static_cast<uint32_t>(cmd.compute.dimension.y);
			uint32_t tz = static_cast<uint32_t>(cmd.compute.dimension.z);
			vkCmdDispatch(commandBuffer, tx, ty, tz);
			break;
		}
	
		case VulkanCommand::Type::Debug:
		{
			debug::debugBreak();
			break;
		}
		
		default:
			ET_FAIL("Not implemented");
		}
	}

	VULKAN_CALL(vkEndCommandBuffer(commandBuffer));
}

uint32_t VulkanRenderPass::recordedFrameIndex() const
{
	return _private->frameIndex;
}

void VulkanRenderPassPrivate::generateDynamicDescriptorSet(RenderPass* pass)
{
	VkDescriptorSetLayoutBinding bindings[] = { {}, {} };
	bindings[0] = { ObjectVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 };
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1] = { MaterialVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 };
	bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetLayoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
	descriptorSetLayoutCreateInfo.pBindings = bindings;
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &descriptorSetLayoutCreateInfo, nullptr, &dynamicDescriptorSetLayout));

	VulkanBuffer::Pointer cb = renderer->sharedConstantBuffer().buffer();
	VkDescriptorBufferInfo objectBufferInfo = { cb->nativeBuffer().buffer, 0, sizeof(mat4) * ObjectVariable_max };
	VkDescriptorBufferInfo materialBufferInfo = { cb->nativeBuffer().buffer, 0, sizeof(mat4) * MaterialVariable_max };

	VkDescriptorSetAllocateInfo descriptorAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorAllocInfo.pSetLayouts = &dynamicDescriptorSetLayout;
	descriptorAllocInfo.descriptorPool = vulkan.descriptorPool;
	descriptorAllocInfo.descriptorSetCount = 1;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &descriptorAllocInfo, &dynamicDescriptorSet));

	VkWriteDescriptorSet writeSets[] = { { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET } };
	{
		writeSets[0].descriptorCount = bindings[0].descriptorCount;
		writeSets[0].descriptorType = bindings[0].descriptorType;
		writeSets[0].dstBinding = bindings[0].binding;
		writeSets[0].pBufferInfo = &objectBufferInfo;
		writeSets[0].dstSet = dynamicDescriptorSet;
		writeSets[1].descriptorCount = bindings[1].descriptorCount;
		writeSets[1].descriptorType = bindings[1].descriptorType;
		writeSets[1].dstBinding = bindings[1].binding;
		writeSets[1].pBufferInfo = &materialBufferInfo;
		writeSets[1].dstSet = dynamicDescriptorSet;
	}
	uint32_t writeSetsCount = static_cast<uint32_t>(sizeof(writeSets) / sizeof(writeSets[0]));
	vkUpdateDescriptorSets(vulkan.device, writeSetsCount, writeSets, 0, nullptr);
}

void VulkanRenderPass::dispatchCompute(const Compute::Pointer& compute, const vec3i& dim)
{
	VulkanCompute::Pointer vulkanCompute = compute;
	MaterialInstance::Pointer material = compute->material();
	VulkanProgram::Pointer program = material->configuration(info().name).program;

	retain();
	vulkanCompute->build(VulkanRenderPass::Pointer(this));
	release();

	ConstantBufferEntry::Pointer objectVariables;
	if (program->reflection().objectVariablesBufferSize > 0)
	{
		objectVariables = _private->renderer->sharedConstantBuffer().allocate(
			program->reflection().objectVariablesBufferSize, ConstantBufferDynamicAllocation);

		for (const auto& v : sharedVariables())
		{
			const Program::Variable& var = program->reflection().objectVariables[v.first];

			if (v.second.isSet() && var.enabled)
				memcpy(objectVariables->data() + var.offset, v.second.data, v.second.size);
		}
	}

	VulkanComputeStruct cs;
	cs.compute = vulkanCompute;
	cs.textureSet = material->textureSet(info().name);
	cs.imageSet = material->imageSet(info().name);
	cs.dimension = dim;
	cs.dynamicOffsets[0] = objectVariables;
	cs.dynamicOffsets[1] = material->constantBufferData(info().name);
	_private->commands[_private->frameIndex].emplace_back(cs);
}

}
