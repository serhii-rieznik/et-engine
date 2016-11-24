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
	TextureDescription description;
};

VulkanTexture::VulkanTexture(VulkanState& vulkan, const TextureDescription::Pointer& desc)
	: Texture(desc)
{
	ET_PIMPL_INIT(VulkanTexture, vulkan);

	_private->description.size = desc->size;
	_private->description.target = desc->target;
	_private->description.format = desc->format;
	_private->description.layersCount = desc->layersCount;
	_private->description.dataLayout = desc->dataLayout;
	_private->description.mipMapCount = desc->mipMapCount;
	_private->format = vulkan::textureFormatValue(desc->format);
	_private->aspect = isDepthTextureFormat(desc->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	info.extent = { static_cast<uint32_t>(desc->size.x), static_cast<uint32_t>(desc->size.y), 1 };
	info.format = _private->format;
	info.imageType = vulkan::textureTargetToImageType(desc->target);
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.mipLevels = desc->mipMapCount;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	
	if (desc->isRenderTarget)
	{
		info.usage |= isDepthTextureFormat(desc->format) ? 
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
	imageViewInfo.viewType = vulkan::textureTargetToImageViewType(desc->target);
	imageViewInfo.format = vulkan::textureFormatValue(desc->format);
	imageViewInfo.subresourceRange = { _private->aspect, 0, desc->mipMapCount, 0, 1 };
	VULKAN_CALL(vkCreateImageView(vulkan.device, &imageViewInfo, nullptr, &_private->imageView));

	if (desc->data.size() > 0)
	{
		setImageData(desc->data);
	}
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
	regions.reserve(_private->description.mipMapCount);

	VkBufferImageCopy region = { };
	region.imageSubresource = { _private->aspect };
	region.imageSubresource.layerCount = 1;
	region.imageExtent.depth = 1;
	for (uint32_t m = 0; m < _private->description.mipMapCount; ++m)
	{
		region.imageSubresource.mipLevel = m;
		region.imageExtent.width = static_cast<uint32_t>(_private->description.sizeForMipLevel(m).x);
		region.imageExtent.height = static_cast<uint32_t>(_private->description.sizeForMipLevel(m).y);
		region.bufferOffset = _private->description.dataOffsetForMipLevel(m, 0);
		region.bufferImageHeight = region.imageExtent.height;
		regions.emplace_back(region);
	};

	_private->vulkan.executeServiceCommands([&](VkCommandBuffer cmdBuffer)
	{
		vulkan::imageBarrier(_private->vulkan, cmdBuffer, _private->image,
			_private->aspect, 0, VK_ACCESS_TRANSFER_WRITE_BIT, 
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
			0, _private->description.mipMapCount);

		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.nativeBuffer().buffer,
			_private->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()), regions.data());

		vulkan::imageBarrier(_private->vulkan, cmdBuffer, _private->image,
			_private->aspect, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
			0, _private->description.mipMapCount);
	});

	_private->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

const VulkanNativeTexture& VulkanTexture::nativeTexture() const
{
	return *(_private);
}

}
