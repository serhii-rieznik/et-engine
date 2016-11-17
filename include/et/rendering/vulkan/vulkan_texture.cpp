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

class VulkanTexturePrivate
{
public:
	VulkanTexturePrivate(VulkanState& v) :
		vulkan(v)
	{
	}

	VulkanState& vulkan;
	VulkanNativeTexture texture;
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

	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	info.extent = { static_cast<uint32_t>(desc->size.x), static_cast<uint32_t>(desc->size.y), 1 };
	info.format = vulkan::textureFormatValue(desc->format);
	info.imageType = vulkan::textureTargetToImageType(desc->target);
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.mipLevels = desc->mipMapCount;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VULKAN_CALL(vkCreateImage(vulkan.device, &info, nullptr, &_private->texture.image));

	vkGetImageMemoryRequirements(vulkan.device, _private->texture.image, &_private->texture.memoryRequirements);

	VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = _private->texture.memoryRequirements.size;
	allocInfo.memoryTypeIndex = vulkan::getMemoryTypeIndex(vulkan, _private->texture.memoryRequirements.memoryTypeBits, memoryProperties);
	VULKAN_CALL(vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &_private->texture.memory));
	VULKAN_CALL(vkBindImageMemory(vulkan.device, _private->texture.image, _private->texture.memory, 0));

	VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewInfo.image = _private->texture.image;
	imageViewInfo.viewType = vulkan::textureTargetToImageViewType(desc->target);
	imageViewInfo.format = vulkan::textureFormatValue(desc->format);
	imageViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, desc->mipMapCount, 0, 1 };
	VULKAN_CALL(vkCreateImageView(vulkan.device, &imageViewInfo, nullptr, &_private->texture.imageView));

	if (desc->data.size() > 0)
	{
		setImageData(desc->data);
	}
}

VulkanTexture::~VulkanTexture()
{
	ET_PIMPL_FINALIZE(VulkanTexture)
}

void VulkanTexture::setImageData(const BinaryDataStorage& data)
{
	ET_ASSERT(data.size() <= _private->texture.memoryRequirements.size);

	Buffer::Description stagingDesc;
	stagingDesc.initialData = BinaryDataStorage(data.data(), data.size());
	stagingDesc.location = Buffer::Location::Host;
	stagingDesc.usage = Buffer::Usage::Staging;
	stagingDesc.size = data.size();
	VulkanBuffer stagingBuffer(_private->vulkan, stagingDesc);

	_private->vulkan.executeServiceCommands([this, &stagingBuffer](VkCommandBuffer cmdBuffer)
	{
		vulkan::imageBarrier(_private->vulkan, cmdBuffer, _private->texture.image,
			VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, 
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
			0, _private->description.mipMapCount);

		Vector<VkBufferImageCopy> regions;
		regions.reserve(_private->description.mipMapCount);

		VkBufferImageCopy region = { };
		region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT };
		region.imageSubresource.layerCount = 1;
		region.imageExtent.depth = 1;

		uint32_t baseWidth = static_cast<uint32_t>(description()->size.x);
		uint32_t baseHeight = static_cast<uint32_t>(description()->size.y);
		for (uint32_t m = 0; m < _private->description.mipMapCount; ++m)
		{
			region.imageSubresource.mipLevel = m;
			region.imageExtent.width = baseWidth;
			region.imageExtent.height = baseHeight;
			region.bufferOffset = _private->description.dataOffsetForMipLevel(m, 0);
			region.bufferImageHeight = baseHeight;
			baseWidth /= 2;
			baseHeight /= 2;
			regions.emplace_back(region);
		}
		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.nativeBuffer().buffer,
			_private->texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()), regions.data());

		vulkan::imageBarrier(_private->vulkan, cmdBuffer, _private->texture.image,
			VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
			0, _private->description.mipMapCount);
	});

	_private->texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

const VulkanNativeTexture& VulkanTexture::nativeTexture() const
{
	return _private->texture;
}

}
