/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et {
struct VulkanRenderSubpass
{
	VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	VkViewport viewport{ };
	VkRect2D scissor{ };
};

uint64_t framebufferHash(uint32_t imageIndex, uint32_t mipLevel, uint32_t layer) {
	return static_cast<uint64_t>(layer) |
		((static_cast<uint64_t>(mipLevel) & 0xFFFF) << 32) |
		((static_cast<uint64_t>(imageIndex) & 0xFFFF) << 48);
}

class VulkanRenderPassPrivate : public VulkanNativeRenderPass
{
public:
	VulkanRenderPassPrivate(VulkanState& v, VulkanRenderer* r)
		: vulkan(v), renderer(r) {
	}

	VulkanState& vulkan;
	VulkanRenderer* renderer = nullptr;

	std::array<Vector<Object::Pointer>, RendererFrameCount> usedObjects;

	Vector<VkClearValue> clearValues;
	Map<uint64_t, VulkanRenderSubpass> subpasses;
	Vector<VulkanRenderSubpass> subpassSequence;
	uint32_t currentSubpassIndex = InvalidIndex;
	uint32_t frameIndex = 0;
	uint32_t beginQueryIndex = 0;
	uint32_t endQueryIndex = 0;
	uint32_t subframeIndex = InvalidIndex;
	uint64_t buildBeginTime = 0;
	uint64_t buildEndTime = 0;

	std::atomic_bool recording{ false };
	std::atomic_bool renderPassStarted{ false };

	ConstantBufferEntry::Pointer buildObjectVariables(const VulkanProgram::Pointer& program);
	void generateDynamicDescriptorSet(RenderPass* pass);
};

VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, VulkanState& vulkan, const RenderPass::ConstructionInfo& passInfo)
	: RenderPass(renderer, passInfo) {
	ET_PIMPL_INIT(VulkanRenderPass, vulkan, renderer);

	ET_ASSERT(!passInfo.name.empty());

	for (uint32_t i = 0; i < RendererFrameCount; ++i)
		_private->usedObjects[i].reserve(65536);

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
	if (passInfo.depth.targetClass != RenderTarget::Class::Disabled)
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

		if (passInfo.depth.targetClass == RenderTarget::Class::DefaultBuffer)
		{
			attachment.format = vulkan.swapchain.depthFormat;
		}
		else
		{
			ET_ASSERT(passInfo.depth.texture.valid());
			VulkanTexture::Pointer texture = passInfo.depth.texture;
			attachment.format = texture->nativeTexture().format;
		}
		{
			_private->clearValues.emplace_back();
			_private->clearValues.back().depthStencil = { passInfo.depth.clearValue.x };
		}
		depthAttachmentReference.layout = attachment.finalLayout;
		++colorAttachmentIndex;
	}

	for (const RenderTarget& target : passInfo.color)
	{
		if (target.targetClass != RenderTarget::Class::Disabled)
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
			if (target.targetClass == RenderTarget::Class::DefaultBuffer)
			{
				ET_ASSERT(target.storeOperation == FramebufferOperation::Store);
				attachment.format = vulkan.swapchain.surfaceFormat.format;
			}
			else
			{
				ET_ASSERT(target.texture.valid());
				const VulkanTexture::Pointer& texture = target.texture;
				attachment.format = texture->nativeTexture().format;
			}
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
	subpassInfo.pDepthStencilAttachment = (passInfo.depth.targetClass == RenderTarget::Class::Disabled) ? nullptr : &depthAttachmentReference;

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassInfo;

	VULKAN_CALL(vkCreateRenderPass(vulkan.device, &createInfo, nullptr, &_private->renderPass));
}

VulkanRenderPass::~VulkanRenderPass() {
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

const VulkanNativeRenderPass& VulkanRenderPass::nativeRenderPass() const {
	return *(_private);
}

void VulkanRenderPass::begin(const RenderPassBeginInfo& beginInfo) {
	ET_ASSERT(_private->recording == false);

	_private->buildBeginTime = queryCurrentTimeInMicroSeconds();

	_private->subpassSequence.clear();
	_private->frameIndex = _private->renderer->frameIndex();

	Texture::Pointer renderTarget = info().color[0].texture;
	Texture::Pointer depthTarget = info().depth.texture;
	bool useCustomColor = info().color[0].targetClass == RenderTarget::Class::Texture;
	bool useCustomDepth = info().depth.targetClass == RenderTarget::Class::Texture;
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

			if (info().depth.targetClass == RenderTarget::Class::DefaultBuffer)
			{
				attachments.emplace_back(_private->vulkan.swapchain.depthBuffer.imageView);
			}
			else if (info().depth.targetClass == RenderTarget::Class::Texture)
			{
				ET_ASSERT(info().depth.texture.valid());
				VulkanTexture::Pointer texture = info().depth.texture;
				attachments.emplace_back(texture->nativeTexture().imageView({ subpass.level, 1, subpass.layer }));
			}

			for (const RenderTarget& rt : info().color)
			{
				if (rt.targetClass == RenderTarget::Class::DefaultBuffer)
				{
					attachments.emplace_back(_private->vulkan.swapchain.currentFrame().colorView);
				}
				else if (rt.targetClass == RenderTarget::Class::Texture)
				{
					ET_ASSERT(rt.texture.valid());
					VulkanTexture::Pointer texture = rt.texture;
					attachments.emplace_back(texture->nativeTexture().imageView({ subpass.level, 1, subpass.layer }));
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

	_private->currentSubpassIndex = InvalidIndex;
	_private->usedObjects[_private->frameIndex].clear();
	_private->subframeIndex = InvalidIndex;
	_private->renderPassStarted = false;
	_private->recording = true;

	setSharedVariable(ObjectVariable::DeltaTime, application().mainRunLoop().lastFrameTime());
	setSharedVariable(ObjectVariable::ContinuousTime, application().mainRunLoop().time());

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VULKAN_CALL(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
	_private->beginQueryIndex = _private->vulkan.writeTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
}

void VulkanRenderPass::nextSubpass() {
	ET_ASSERT(_private->recording);

	uint32_t nextSubpassIndex = (_private->currentSubpassIndex == InvalidIndex) ? 0 : _private->currentSubpassIndex + 1;

	ET_ASSERT(nextSubpassIndex < _private->subpassSequence.size());

	if (nextSubpassIndex < _private->subpassSequence.size())
	{
		_private->subframeIndex = (_private->subframeIndex == InvalidIndex) ? 0 : (_private->subframeIndex + 1);
		{
			VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
			ET_ASSERT(_private->renderPassStarted == false);

			_private->renderPassStarted = true;
			const VulkanRenderSubpass& subpass = _private->subpassSequence.at(_private->subframeIndex);
			vkCmdBeginRenderPass(commandBuffer, &subpass.beginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetScissor(commandBuffer, 0, 1, &subpass.scissor);
			vkCmdSetViewport(commandBuffer, 0, 1, &subpass.viewport);
		}
		_private->currentSubpassIndex = nextSubpassIndex;

		vec4 viewport(
			_private->subpassSequence[_private->currentSubpassIndex].viewport.x,
			_private->subpassSequence[_private->currentSubpassIndex].viewport.y,
			_private->subpassSequence[_private->currentSubpassIndex].viewport.width,
			_private->subpassSequence[_private->currentSubpassIndex].viewport.height);
		setSharedVariable(ObjectVariable::Viewport, viewport);
	}
}

void VulkanRenderPass::pushRenderBatch(const MaterialInstance::Pointer& inMaterial, const VertexStream::Pointer& vertexStream, uint32_t first, uint32_t count) {
	ET_ASSERT(_private->recording);

	VulkanPipelineState::Pointer pipelineState;
	{
		InstusivePointerScope<VulkanRenderPass> scope(this);
		const Material::Pointer& baseMaterial = inMaterial->base();
		pipelineState = _private->renderer->acquireGraphicsPipeline(VulkanRenderPass::Pointer(this), baseMaterial, vertexStream);
	}

	if (pipelineState->nativePipeline().pipeline == nullptr)
		return;

	bool hasVertexBuffer = vertexStream.valid() && vertexStream->vertexBuffer().valid();
	bool hasIndexBuffer = vertexStream.valid() && vertexStream->indexBuffer().valid();

	MaterialInstance::Pointer material = inMaterial;
	for (const auto& sh : sharedTextures())
	{
		material->setTexture(sh.first, sh.second.first);
		material->setSampler(sh.first, sh.second.second);
	}
	Vector<Object::Pointer>& usedObjects = _private->usedObjects[_private->frameIndex];
	usedObjects.reserve(_private->usedObjects[_private->frameIndex].size() + 6);
	usedObjects.emplace_back(pipelineState);

	usedObjects.emplace_back(material->constantBufferData(info().name));
	ConstantBufferEntry* materialVariables = static_cast<ConstantBufferEntry*>(usedObjects.back().pointer());

	usedObjects.emplace_back(buildObjectVariables(pipelineState->program()));
	ConstantBufferEntry* objectVariables = static_cast<ConstantBufferEntry*>(usedObjects.back().pointer());

	usedObjects.emplace_back(material->textureSet(info().name));
	VulkanTextureSet* textureSet = static_cast<VulkanTextureSet*>(usedObjects.back().pointer());

	usedObjects.emplace_back(material->imageSet(info().name));
	VulkanTextureSet* imageSet = static_cast<VulkanTextureSet*>(usedObjects.back().pointer());

	VkDescriptorSet descriptorSets[DescriptorSetClass_Count] = {
		_private->dynamicDescriptorSet,
		textureSet->nativeSet().descriptorSet,
		imageSet->nativeSet().descriptorSet,
	};

	uint32_t dynamicOffsets[DescriptorSetClass::DynamicDescriptorsCount] = {
		static_cast<uint32_t>(objectVariables != nullptr ? objectVariables->offset() : 0),
		static_cast<uint32_t>(materialVariables != nullptr ? materialVariables->offset() : 0)
	};

	ET_ASSERT(_private->renderPassStarted);

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState->nativePipeline().pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState->nativePipeline().layout, 0,
		DescriptorSetClass_Count, descriptorSets, DescriptorSetClass::DynamicDescriptorsCount, dynamicOffsets);

	if (hasVertexBuffer)
	{
		usedObjects.emplace_back(vertexStream->vertexBuffer());
		VulkanBuffer* vertexBuffer = static_cast<VulkanBuffer*>(usedObjects.back().pointer());

		VkDeviceSize offsets[] = { 0 };
		VkBuffer buffers[] = { vertexBuffer->nativeBuffer().buffer };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	}

	if (hasIndexBuffer)
	{
		usedObjects.emplace_back(vertexStream->indexBuffer());
		VulkanBuffer* indexBuffer = static_cast<VulkanBuffer*>(usedObjects.back().pointer());

		VkIndexType indexType = vulkan::indexBufferFormat(vertexStream->indexArrayFormat());
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->nativeBuffer().buffer, 0, indexType);
		vkCmdDrawIndexed(commandBuffer, count, 1, first, 0, 0);
	}
	else
	{
		vkCmdDraw(commandBuffer, count, 1, first, 0);
	}
}

void VulkanRenderPass::dispatchCompute(const Compute::Pointer& compute, const vec3i& dim) {
	MaterialInstance::Pointer material = compute->material();
	ET_ASSERT(material->isInstance());

	VulkanCompute::Pointer vulkanCompute = compute;
	VulkanProgram::Pointer program = material->base()->configuration(info().name).program;
	{
		InstusivePointerScope<VulkanRenderPass> scope(this);
		vulkanCompute->build(VulkanRenderPass::Pointer(this));
	}

	VulkanTextureSet::Pointer textureSet = material->textureSet(info().name);
	VulkanTextureSet::Pointer imageSet = material->imageSet(info().name);
	ConstantBufferEntry::Pointer materialVariables = material->constantBufferData(info().name);
	ConstantBufferEntry::Pointer objectVariables = buildObjectVariables(program);

	_private->usedObjects[_private->frameIndex].emplace_back(textureSet);
	_private->usedObjects[_private->frameIndex].emplace_back(imageSet);
	_private->usedObjects[_private->frameIndex].emplace_back(materialVariables);
	_private->usedObjects[_private->frameIndex].emplace_back(objectVariables);

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;

	VkDescriptorSet descriptorSets[DescriptorSetClass_Count] = {
		_private->dynamicDescriptorSet,
		textureSet->nativeSet().descriptorSet,
		imageSet->nativeSet().descriptorSet,
	};

	uint32_t dynamicOffsets[DescriptorSetClass::DynamicDescriptorsCount] = {
		static_cast<uint32_t>(objectVariables.valid() ? objectVariables->offset() : 0),
		static_cast<uint32_t>(materialVariables.valid() ? materialVariables->offset() : 0)
	};

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanCompute->nativeCompute().pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanCompute->nativeCompute().layout,
		0, DescriptorSetClass_Count, descriptorSets, DescriptorSetClass::DynamicDescriptorsCount, dynamicOffsets);

	uint32_t tx = static_cast<uint32_t>(dim.x);
	uint32_t ty = static_cast<uint32_t>(dim.y);
	uint32_t tz = static_cast<uint32_t>(dim.z);
	vkCmdDispatch(commandBuffer, tx, ty, tz);
}

void VulkanRenderPass::pushImageBarrier(const Texture::Pointer& texture, const ResourceBarrier& resourceBarrier) {
	ET_ASSERT(_private->recording);

	_private->usedObjects[_private->frameIndex].emplace_back(texture);

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
	VulkanTexture::Pointer tex = texture;
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.image = tex->nativeTexture().image;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = vulkan::texureStateToAccessFlags(resourceBarrier.toState);
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = vulkan::texureStateToImageLayout(resourceBarrier.toState);
	barrier.srcQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
	barrier.dstQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
	barrier.subresourceRange = { tex->nativeTexture().aspect, resourceBarrier.range.firstLevel,
		resourceBarrier.range.levelCount, resourceBarrier.range.firstLayer, resourceBarrier.range.layerCount };

	vkCmdPipelineBarrier(commandBuffer, vulkan::accessMaskToPipelineStage(barrier.srcAccessMask),
		vulkan::accessMaskToPipelineStage(barrier.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanRenderPass::copyImage(const Texture::Pointer& texFrom, const Texture::Pointer& texTo, const CopyDescriptor& desc) {
	ET_ASSERT(_private->recording);

	_private->usedObjects[_private->frameIndex].emplace_back(texFrom);
	_private->usedObjects[_private->frameIndex].emplace_back(texTo);

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
	VulkanTexture::Pointer tFrom = texFrom;
	VulkanTexture::Pointer tTo = texTo;
	VkImageCopy region = {};
	region.srcSubresource.aspectMask = tFrom->nativeTexture().aspect;
	region.srcSubresource.baseArrayLayer = desc.layerFrom;
	region.srcSubresource.layerCount = 1;
	region.srcSubresource.mipLevel = desc.levelFrom;
	region.srcOffset.x = desc.offsetFrom.x;
	region.srcOffset.y = desc.offsetFrom.y;
	region.srcOffset.z = desc.offsetFrom.z;
	region.dstSubresource.aspectMask = tTo->nativeTexture().aspect;
	region.dstSubresource.baseArrayLayer = desc.layerTo;
	region.dstSubresource.layerCount = 1;
	region.dstSubresource.mipLevel = desc.levelTo;
	region.dstOffset.x = desc.offsetTo.x;
	region.dstOffset.y = desc.offsetTo.y;
	region.dstOffset.z = desc.offsetTo.z;
	region.extent.width = desc.size.x;
	region.extent.height = desc.size.y;
	region.extent.depth = desc.size.z;
	vkCmdCopyImage(commandBuffer, tFrom->nativeTexture().image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		tTo->nativeTexture().image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanRenderPass::copyImageToBuffer(const Texture::Pointer& image, const Buffer::Pointer& buffer, const CopyDescriptor& desc) {
	ET_ASSERT(_private->recording);

	VulkanTexture::Pointer tex = image;
	VulkanBuffer::Pointer buf = buffer;
	_private->usedObjects[_private->frameIndex].emplace_back(tex);
	_private->usedObjects[_private->frameIndex].emplace_back(buf);

	VkBufferImageCopy region = {};
	region.imageExtent = { std::max<uint32_t>(0u, desc.size.x), std::max<uint32_t>(0u, desc.size.y), std::max<uint32_t>(0u, desc.size.z) };
	region.imageOffset.x = desc.offsetFrom.x;
	region.imageOffset.y = desc.offsetFrom.y;
	region.imageOffset.z = desc.offsetFrom.z;
	region.imageSubresource.aspectMask = tex->nativeTexture().aspect;
	region.imageSubresource.baseArrayLayer = desc.layerFrom;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = desc.levelFrom;
	region.bufferOffset = desc.bufferOffsetTo;

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
	vkCmdCopyImageToBuffer(commandBuffer, tex->nativeTexture().image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		buf->nativeBuffer().buffer, 1, &region);
}

void VulkanRenderPass::endSubpass() {
	ET_ASSERT(_private->recording);

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
	ET_ASSERT(_private->renderPassStarted == true);
	vkCmdEndRenderPass(commandBuffer);
	_private->renderPassStarted = false;
}

void VulkanRenderPass::end() {
	ET_ASSERT(_private->recording);

	VkCommandBuffer commandBuffer = _private->content[_private->frameIndex].commandBuffer;
	_private->endQueryIndex = _private->vulkan.writeTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	VULKAN_CALL(vkEndCommandBuffer(commandBuffer));

	_private->recording = false;
	_private->buildEndTime = queryCurrentTimeInMicroSeconds();
}

void VulkanRenderPass::debug() {
	debug::debugBreak();
}

ConstantBufferEntry::Pointer VulkanRenderPass::buildObjectVariables(const VulkanProgram::Pointer& program) {
	ConstantBufferEntry::Pointer result;
	if (program->reflection().objectVariablesBufferSize > 0)
	{
		result = _private->renderer->sharedConstantBuffer().allocate(
			program->reflection().objectVariablesBufferSize, ConstantBufferDynamicAllocation);

		for (const auto& v : sharedVariables())
		{
			if (v.first == uint32_t(ObjectVariable::EnvironmentSphericalHarmonics))
				printf(".");

			const Program::Variable& var = program->reflection().objectVariables[v.first];
			if (var.enabled && v.second.isSet())
			{
				ET_ASSERT(v.second.elementCount <= var.arraySize);
				memcpy(result->data() + var.offset, v.second.data, v.second.dataSize);
			}
		}
	}
	return result;
}

bool VulkanRenderPass::fillStatistics(uint64_t* buffer, RenderPassStatistics& stat) {
	uint32_t validBits = _private->vulkan.queues[VulkanQueueClass::Graphics].properties.timestampValidBits;

	uint64_t timestampMask = 0;
	for (uint64_t i = 0; i < validBits; ++i)
		timestampMask |= (1llu << i);

	uint64_t beginTime = buffer[_private->beginQueryIndex] & timestampMask;
	uint64_t endTime = buffer[_private->endQueryIndex] & timestampMask;

	double periodDuration = static_cast<double>(_private->vulkan.physicalDeviceProperties.limits.timestampPeriod);
	double periods = static_cast<double>(endTime - beginTime);

	strncpy(stat.name, info().name.c_str(), std::min(static_cast<size_t>(MaxRenderPassName), info().name.size()));
	stat.gpuExecution = static_cast<uint64_t>((periods * periodDuration) / 1000.0);
	stat.cpuBuild = _private->buildEndTime - _private->buildBeginTime;

	return true;
}

/*
 * Private implementation
 */
void VulkanRenderPassPrivate::generateDynamicDescriptorSet(RenderPass* pass) {
	VkDescriptorSetLayoutBinding bindings[] = { {},{} };
	bindings[0] = { ObjectVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 };
	bindings[0].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[1] = { MaterialVariablesBufferIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 };
	bindings[1].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;

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

}
