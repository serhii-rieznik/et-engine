/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanTexturePrivate : public VulkanNativeTexture
{
public:
	VulkanTexturePrivate(VulkanState& v) :
		vulkan(v) { }

	VulkanState& vulkan;
};

VulkanTexture::VulkanTexture(VulkanState& vulkan, const Description& desc, const BinaryDataStorage& data)
	: Texture(desc)
{
	ET_PIMPL_INIT(VulkanTexture, vulkan);

	_private->format = vulkan::textureFormatValue(desc.format);
	_private->aspect = isDepthTextureFormat(desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	info.extent = { static_cast<uint32_t>(desc.size.x), static_cast<uint32_t>(desc.size.y), 1 };
	info.format = _private->format;
	info.imageType = vulkan::textureTargetToImageType(desc.target);
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.mipLevels = desc.mipMapCount;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	
	if (desc.isRenderTarget)
	{
		info.usage |= isDepthTextureFormat(desc.format) ? 
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	
	VULKAN_CALL(vkCreateImage(vulkan.device, &info, nullptr, &_private->image));

	vkGetImageMemoryRequirements(vulkan.device, _private->image, &_private->memoryRequirements);

	VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = _private->memoryRequirements.size;
	allocInfo.memoryTypeIndex = vulkan::getMemoryTypeIndex(vulkan, _private->memoryRequirements.memoryTypeBits, memoryProperties);
	VULKAN_CALL(vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &_private->memory));
	VULKAN_CALL(vkBindImageMemory(vulkan.device, _private->image, _private->memory, 0));

	VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewInfo.image = _private->image;
	imageViewInfo.viewType = vulkan::textureTargetToImageViewType(desc.target);
	imageViewInfo.format = vulkan::textureFormatValue(desc.format);
	imageViewInfo.subresourceRange = { _private->aspect, 0, desc.mipMapCount, 0, 1 };
	VULKAN_CALL(vkCreateImageView(vulkan.device, &imageViewInfo, nullptr, &_private->imageView));

	if (data.size() > 0)
		setImageData(data);
}

VulkanTexture::~VulkanTexture()
{
	vkDestroyImageView(_private->vulkan.device, _private->imageView, nullptr);
	vkDestroyImage(_private->vulkan.device, _private->image, nullptr);
	vkFreeMemory(_private->vulkan.device, _private->memory, nullptr);
	ET_PIMPL_FINALIZE(VulkanTexture)
}

void VulkanTexture::setImageData(const BinaryDataStorage& data)
{
	ET_ASSERT(data.size() <= _private->memoryRequirements.size);

	Buffer::Description stagingDesc;
	stagingDesc.initialData = BinaryDataStorage(data.data(), data.size());
	stagingDesc.location = Buffer::Location::Host;
	stagingDesc.usage = Buffer::Usage::Staging;
	stagingDesc.size = data.size();
	VulkanBuffer stagingBuffer(_private->vulkan, stagingDesc);

	Vector<VkBufferImageCopy> regions;
	regions.reserve(description().mipMapCount);

	VkBufferImageCopy region = { };
	region.imageSubresource = { _private->aspect };
	region.imageSubresource.layerCount = 1;
	region.imageExtent.depth = 1;
	for (uint32_t m = 0; m < description().mipMapCount; ++m)
	{
		region.imageSubresource.mipLevel = m;
		region.imageExtent.width = static_cast<uint32_t>(description().sizeForMipLevel(m).x);
		region.imageExtent.height = static_cast<uint32_t>(description().sizeForMipLevel(m).y);
		region.bufferOffset = description().dataOffsetForMipLevel(m, 0);
		region.bufferImageHeight = region.imageExtent.height;
		regions.emplace_back(region);
	};

	_private->vulkan.executeServiceCommands([&](VkCommandBuffer cmdBuffer)
	{
		vulkan::imageBarrier(_private->vulkan, cmdBuffer, _private->image,
			_private->aspect, 0, VK_ACCESS_TRANSFER_WRITE_BIT, 
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
			0, description().mipMapCount);

		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.nativeBuffer().buffer,
			_private->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()), regions.data());

		vulkan::imageBarrier(_private->vulkan, cmdBuffer, _private->image,
			_private->aspect, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
			0, description().mipMapCount);
	});

	_private->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void VulkanTexture::updateRegion(const vec2i & pos, const vec2i & size, const BinaryDataStorage& data)
{
	ET_ASSERT(pos.x >= 0);
	ET_ASSERT(pos.x + size.x < description().size.x);
	ET_ASSERT(pos.y >= 0);
	ET_ASSERT(pos.y + size.y < description().size.y);

	uint32_t dataSize = static_cast<uint32_t>(size.square()) * bitsPerPixelForTextureFormat(description().format) / 8;
	ET_ASSERT(dataSize <= data.size());

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
	
	_private->vulkan.executeServiceCommands([&](VkCommandBuffer cmdBuffer)
	{
		vulkan::imageBarrier(_private->vulkan, cmdBuffer, _private->image, _private->aspect, 
			0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1);

		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.nativeBuffer().buffer,
			_private->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		vulkan::imageBarrier(_private->vulkan, cmdBuffer, _private->image, _private->aspect, 
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1);
	});
}

const VulkanNativeTexture& VulkanTexture::nativeTexture() const
{
	return *(_private);
}

}
