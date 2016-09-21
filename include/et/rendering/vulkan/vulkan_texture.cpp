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
		vulkan(v) { }

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
	info.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	info.mipLevels = desc->mipMapCount;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VULKAN_CALL(vkCreateImage(vulkan.device, &info, nullptr, &_private->texture.image));

	vkGetImageMemoryRequirements(vulkan.device, _private->texture.image, &_private->texture.memoryRequirements);

	VkMemoryPropertyFlags memoryProperties = (desc->data.size() == 0) 
		? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
		: VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = _private->texture.memoryRequirements.size;
	allocInfo.memoryTypeIndex = vulkan::getMemoryTypeIndex(vulkan, _private->texture.memoryRequirements.memoryTypeBits, memoryProperties);
	VULKAN_CALL(vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &_private->texture.memory));

	VULKAN_CALL(vkBindImageMemory(vulkan.device, _private->texture.image, _private->texture.memory, 0));

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

}


}
