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
};

VulkanTexture::VulkanTexture(VulkanState& vulkan, TextureDescription::Pointer desc)
	: Texture(desc)
{
	ET_PIMPL_INIT(VulkanTexture, vulkan);

	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	info.arrayLayers = 1;
	info.extent = { static_cast<uint32_t>(desc->size.x), static_cast<uint32_t>(desc->size.y), 1 };
	info.format = vulkan::textureFormatValue(desc->format);
	info.imageType = vulkan::textureTargetToImageType(desc->target);
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.mipLevels = desc->mipMapCount;
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
	imageViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VULKAN_CALL(vkCreateImageView(vulkan.device, &imageViewInfo, nullptr, &_private->texture.imageView));

	VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VULKAN_CALL(vkCreateSampler(vulkan.device, &samplerInfo, nullptr, &_private->texture.sampler));

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
	VulkanNativeBuffer stagingBuffer(_private->vulkan, static_cast<uint32_t>(_private->texture.memoryRequirements.size), 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
	void* ptr = stagingBuffer.map(0, data.size());
	memcpy(ptr, data.data(), data.size());
	stagingBuffer.unmap();

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VULKAN_CALL(vkBeginCommandBuffer(_private->vulkan.serviceCommandBuffer, &beginInfo));
	{
		vulkan::imageBarrier(_private->vulkan, _private->vulkan.serviceCommandBuffer, _private->texture.image,
			0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkBufferImageCopy region = { };
		region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.imageExtent.width = static_cast<uint32_t>(description()->size.x);
		region.imageExtent.height = static_cast<uint32_t>(description()->size.y);
		region.imageExtent.depth = 1;
		vkCmdCopyBufferToImage(_private->vulkan.serviceCommandBuffer, stagingBuffer.buffer(), 
			_private->texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		vulkan::imageBarrier(_private->vulkan, _private->vulkan.serviceCommandBuffer, _private->texture.image,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}
	VULKAN_CALL(vkEndCommandBuffer(_private->vulkan.serviceCommandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_private->vulkan.serviceCommandBuffer;
	VULKAN_CALL(vkQueueSubmit(_private->vulkan.queue, 1, &submitInfo, nullptr));
	VULKAN_CALL(vkQueueWaitIdle(_private->vulkan.queue));
}


}
