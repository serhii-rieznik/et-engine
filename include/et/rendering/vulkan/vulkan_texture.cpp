/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan_buffer.h>
#include <et/rendering/vulkan/vulkan.h>

#define ET_ENABLE_TEXTURE_VALIDATION 1

namespace et {

class VulkanTexturePrivate : public VulkanNativeTexture
{
public:
	VulkanTexturePrivate(VulkanState& v) :
		VulkanNativeTexture(v) {
	}

	uint32_t mappedState = 0;
};

VulkanTexture::VulkanTexture(VulkanState& vulkan, const Description& desc, const BinaryDataStorage& data)
	: Texture(desc) {
	ET_PIMPL_INIT(VulkanTexture, vulkan);

	_private->format = vulkan::textureFormatValue(desc.format);
	_private->aspect = isDepthTextureFormat(desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	_private->imageViewType = vulkan::textureTargetToImageViewType(desc.target);

	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	info.extent = { static_cast<uint32_t>(desc.size.x), static_cast<uint32_t>(desc.size.y), 1 };
	info.format = _private->format;
	info.imageType = vulkan::textureTargetToImageType(desc.target);
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.mipLevels = desc.levelCount;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.tiling = (desc.flags & Texture::Flags::Readback) ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
	info.usage = 0;

	switch (desc.target)
	{
	case TextureTarget::Texture_2D:
		info.arrayLayers = 1;
		break;
	case TextureTarget::Texture_Cube:
		ET_ASSERT(desc.layerCount == 6);
		info.arrayLayers = 6;
		info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		break;
	case TextureTarget::Texture_2D_Array:
		info.arrayLayers = desc.layerCount;
		info.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
		break;
	default:
		ET_FAIL("Invalid TextureTarget provided");
	}
	_private->layerCount = info.arrayLayers;
	_private->levelCount = info.mipLevels;

	if (desc.flags & Texture::Flags::ShaderResource)
		info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

	if (desc.flags & Texture::Flags::RenderTarget)
		info.usage |= isDepthTextureFormat(desc.format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (desc.flags & Texture::Flags::Storage)
		info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if (desc.flags & Texture::Flags::CopySource)
		info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if ((desc.flags & Texture::Flags::CopyDestination) || (data.size() > 0))
		info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

#if (ET_ENABLE_TEXTURE_VALIDATION)
	{
		VkImageFormatProperties props = {};
		vkGetPhysicalDeviceImageFormatProperties(vulkan.physicalDevice, info.format, info.imageType,
			info.tiling, info.usage, info.flags, &props);
		ET_ASSERT(info.extent.width <= props.maxExtent.width);
		ET_ASSERT(info.extent.height <= props.maxExtent.height);
		ET_ASSERT(info.extent.depth <= props.maxExtent.depth);
	}
#endif
	VULKAN_CALL(vkCreateImage(vulkan.device, &info, nullptr, &_private->image));

	vkGetImageMemoryRequirements(vulkan.device, _private->image, &_private->memoryRequirements);

	VkMemoryPropertyFlags memoryProperties = (desc.flags & Texture::Flags::Readback) ?
		(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) :
		(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	uint32_t typeIndex = vulkan::getMemoryTypeIndex(vulkan, _private->memoryRequirements.memoryTypeBits, memoryProperties);
	if (_private->vulkan.allocator.allocateExclusiveMemory(_private->memoryRequirements.size, typeIndex, _private->allocation))
	{
		VULKAN_CALL(vkBindImageMemory(vulkan.device, _private->image, _private->allocation.memory, _private->allocation.offset));
	}

	if (data.size() > 0)
		setImageData(data);
}

VulkanTexture::~VulkanTexture() {
	for (auto imageView : _private->allImageViews)
		vkDestroyImageView(_private->vulkan.device, imageView.second, nullptr);

	vkDestroyImage(_private->vulkan.device, _private->image, nullptr);
	_private->vulkan.allocator.release(_private->allocation);

	ET_PIMPL_FINALIZE(VulkanTexture);
}

void VulkanTexture::setImageData(const BinaryDataStorage& data) {
	ET_ASSERT(data.size() <= _private->memoryRequirements.size);

	Buffer::Description stagingDesc;
	stagingDesc.initialData = BinaryDataStorage(data.data(), data.size());
	stagingDesc.location = Buffer::Location::Host;
	stagingDesc.usage = Buffer::Usage::Staging;
	stagingDesc.size = data.size();
	VulkanBuffer stagingBuffer(_private->vulkan, stagingDesc);

	Vector<VkBufferImageCopy> regions;
	regions.reserve(description().levelCount);

	VkBufferImageCopy region = { };
	region.imageSubresource = { _private->aspect };
	region.imageSubresource.layerCount = 1;
	region.imageExtent.depth = 1;
	for (uint32_t m = 0; m < description().levelCount; ++m)
	{
		region.imageSubresource.mipLevel = m;
		region.imageExtent.width = static_cast<uint32_t>(description().sizeForMipLevel(m).x);
		region.imageExtent.height = static_cast<uint32_t>(description().sizeForMipLevel(m).y);
		region.bufferOffset = description().dataOffsetForMipLevel(m, 0);
		region.bufferImageHeight = region.imageExtent.height;
		regions.emplace_back(region);
	};

	_private->vulkan.executeServiceCommands(VulkanQueueClass::Graphics, [&](VkCommandBuffer cmdBuffer) {
		VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrierInfo.srcAccessMask = 0;
		barrierInfo.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrierInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrierInfo.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrierInfo.srcQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
		barrierInfo.dstQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
		barrierInfo.subresourceRange = { _private->aspect, 0, description().levelCount, 0, description().layerCount };
		barrierInfo.image = _private->image;
		vkCmdPipelineBarrier(cmdBuffer, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
			vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);

		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.nativeBuffer().buffer,
			_private->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()), regions.data());

		barrierInfo.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrierInfo.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrierInfo.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrierInfo.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(cmdBuffer, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
			vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
	});
}

void VulkanTexture::updateRegion(const vec2i & pos, const vec2i & size, const BinaryDataStorage& data) {
	ET_ASSERT(pos.x >= 0);
	ET_ASSERT(pos.x + size.x < description().size.x);
	ET_ASSERT(pos.y >= 0);
	ET_ASSERT(pos.y + size.y < description().size.y);

	ET_ASSERT((size.square() * bitsPerPixelForTextureFormat(description().format) / 8) <= data.size());

	Buffer::Description stagingDesc;
	stagingDesc.initialData = BinaryDataStorage(data.data(), data.size());
	stagingDesc.location = Buffer::Location::Host;
	stagingDesc.usage = Buffer::Usage::Staging;
	stagingDesc.size = data.size();
	VulkanBuffer stagingBuffer(_private->vulkan, stagingDesc);

	VkBufferImageCopy region = { };
	region.imageSubresource = { _private->aspect };
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = static_cast<uint32_t>(pos.x);
	region.imageOffset.y = static_cast<uint32_t>(pos.y);
	region.imageExtent.width = static_cast<uint32_t>(size.x);
	region.imageExtent.height = static_cast<uint32_t>(size.y);
	region.imageExtent.depth = 1;

	_private->vulkan.executeServiceCommands(VulkanQueueClass::Graphics, [&](VkCommandBuffer cmdBuffer) {
		VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrierInfo.srcAccessMask = 0;
		barrierInfo.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrierInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrierInfo.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrierInfo.srcQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
		barrierInfo.dstQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
		barrierInfo.subresourceRange = { _private->aspect, 0, description().levelCount, 0, description().layerCount };
		barrierInfo.image = _private->image;
		vkCmdPipelineBarrier(cmdBuffer, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
			vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);

		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.nativeBuffer().buffer,
			_private->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		barrierInfo.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrierInfo.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrierInfo.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrierInfo.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(cmdBuffer, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
			vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
	});
}

uint8_t* VulkanTexture::map(uint32_t level, uint32_t layer, uint32_t options) {
	// TODO : more intelligent mapping
	ET_ASSERT(description().flags & Texture::Flags::Readback);
	ET_ASSERT(_private->mappedState == 0);
	_private->mappedState = options;

	_private->vulkan.executeServiceCommands(VulkanQueueClass::Graphics, [&](VkCommandBuffer cmdBuffer) {
		VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrierInfo.srcAccessMask = 0;
		barrierInfo.dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
		barrierInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrierInfo.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrierInfo.srcQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
		barrierInfo.dstQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
		barrierInfo.subresourceRange = { _private->aspect, 0, description().levelCount, 0, description().layerCount };
		barrierInfo.image = _private->image;

		vkCmdPipelineBarrier(cmdBuffer, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
			vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
	});

	VkDeviceSize offset = description().dataOffsetForLayer(layer, level);
	return _private->vulkan.allocator.map(_private->allocation) + offset;
}

void VulkanTexture::unmap() {
	ET_ASSERT(_private->mappedState != 0);
	_private->vulkan.allocator.unmap(_private->allocation);

	_private->vulkan.executeServiceCommands(VulkanQueueClass::Graphics, [&](VkCommandBuffer cmdBuffer) {
		VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrierInfo.srcAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
		barrierInfo.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrierInfo.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrierInfo.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrierInfo.srcQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
		barrierInfo.dstQueueFamilyIndex = _private->vulkan.queues[VulkanQueueClass::Graphics].index;
		barrierInfo.subresourceRange = { _private->aspect, 0, description().levelCount, 0, description().layerCount };
		barrierInfo.image = _private->image;
		vkCmdPipelineBarrier(cmdBuffer, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
			vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
	});
	_private->mappedState = 0;
}

const VulkanNativeTexture& VulkanTexture::nativeTexture() const {
	return *(_private);
}

VulkanNativeTexture& VulkanTexture::nativeTexture() {
	return *(_private);
}

}
