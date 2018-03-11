/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_textureset.h>
#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan_sampler.h>
#include <et/rendering/vulkan/vulkan_renderer.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanTextureSetPrivate : public VulkanNativeTextureSet
{
public:
	VulkanTextureSetPrivate(VulkanState& v) :
		vulkan(v) { }

	VulkanState& vulkan;
};

VulkanTextureSet::VulkanTextureSet(VulkanRenderer* renderer, VulkanState& vulkan, const Description& desc)
{
	ET_PIMPL_INIT(VulkanTextureSet, vulkan);

	const uint32_t MaxObjectsCount = 32;
	std::array<VkDescriptorSetLayoutBinding, MaxObjectsCount> imageBindings = {};
	std::array<VkWriteDescriptorSet, MaxObjectsCount> imageWriteSet = {};
	std::array<VkDescriptorImageInfo, MaxObjectsCount> imageInfos = {};

	std::array<VkDescriptorSetLayoutBinding, MaxObjectsCount> samplerBindings = {};
	std::array<VkWriteDescriptorSet, MaxObjectsCount> samplerWriteSet = {};
	std::array<VkDescriptorImageInfo, MaxObjectsCount> samplerInfos = {};

	uint32_t imagesCount = 0;
	uint32_t samplersCount = 0;
	for (const auto& entry : desc)
	{
		VkShaderStageFlagBits stageFlags = vulkan::programStageValue(entry.first);

		for (const auto& e : entry.second.textures)
		{
			VkDescriptorSetLayoutBinding& binding = imageBindings[imagesCount];
			binding.binding = e.first;
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			binding.stageFlags = stageFlags;

			VkDescriptorImageInfo& info = imageInfos[imagesCount];
			info.imageView = VulkanTexture::Pointer(e.second.image)->nativeTexture().imageView(e.second.range);
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet& ws = imageWriteSet[imagesCount];
			ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ws.descriptorCount = binding.descriptorCount;
			ws.descriptorType = binding.descriptorType;
			ws.dstBinding = binding.binding;
			ws.pImageInfo = imageInfos.data() + imagesCount;

			++imagesCount;
			ET_ASSERT(imagesCount < MaxObjectsCount);
		}

		for (const auto& e : entry.second.images)
		{
			VkDescriptorSetLayoutBinding& binding = imageBindings[imagesCount];
			binding.binding = e.first;
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.stageFlags = stageFlags;

			// TODO : use range for images (instead of whole range)
			VkDescriptorImageInfo& info = imageInfos[imagesCount];
			info.imageView = VulkanTexture::Pointer(e.second)->nativeTexture().imageView(ResourceRange::whole);
			info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			VkWriteDescriptorSet& ws = imageWriteSet[imagesCount];
			ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ws.descriptorCount = binding.descriptorCount;
			ws.descriptorType = binding.descriptorType;
			ws.dstBinding = binding.binding;
			ws.pImageInfo = imageInfos.data() + imagesCount;

			++imagesCount;
			ET_ASSERT(imagesCount < MaxObjectsCount);
		}

		for (const auto& e : entry.second.samplers)
		{
			VkDescriptorSetLayoutBinding& binding = samplerBindings[samplersCount];
			binding.binding = e.first;
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			binding.stageFlags = stageFlags;

			VkDescriptorImageInfo& info = imageInfos[samplersCount];
			info.sampler = VulkanSampler::Pointer(e.second)->nativeSampler().sampler;

			VkWriteDescriptorSet& ws = samplerWriteSet[samplersCount];
			ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ws.descriptorCount = binding.descriptorCount;
			ws.descriptorType = binding.descriptorType;
			ws.dstBinding = binding.binding;
			ws.pImageInfo = imageInfos.data() + samplersCount;
			
			++samplersCount;
			ET_ASSERT(samplersCount < MaxObjectsCount);
		}
	}

	// if (objectsCount > 0)
	{
		VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		createInfo.bindingCount = imagesCount;
		createInfo.pBindings = imageBindings.data();
		VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &createInfo, nullptr, &_private->texturesSetLayout));

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = vulkan.descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_private->texturesSetLayout;
		VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &allocInfo, &_private->texturesSet));

		for (uint32_t i = 0; i < imagesCount; ++i)
			imageWriteSet[i].dstSet = _private->texturesSet;

		vkUpdateDescriptorSets(vulkan.device, imagesCount, imageWriteSet.data(), 0, nullptr);
	}

	// if (samplersCount > 0)
	{
		VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		createInfo.bindingCount = samplersCount;
		createInfo.pBindings = samplerBindings.data();
		VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &createInfo, nullptr, &_private->samplersSetLayout));

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = vulkan.descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_private->samplersSetLayout;
		VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &allocInfo, &_private->samplersSet));

		for (uint32_t i = 0; i < samplersCount; ++i)
			samplerWriteSet[i].dstSet = _private->samplersSet;

		vkUpdateDescriptorSets(vulkan.device, samplersCount, samplerWriteSet.data(), 0, nullptr);
	}
}

VulkanTextureSet::~VulkanTextureSet()
{
	VULKAN_CALL(vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptorPool, 1, &_private->texturesSet));
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->texturesSetLayout, nullptr);
	
	VULKAN_CALL(vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptorPool, 1, &_private->samplersSet));
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->samplersSetLayout, nullptr);

	ET_PIMPL_FINALIZE(VulkanTextureSet);
}

const VulkanNativeTextureSet& VulkanTextureSet::nativeSet() const
{
	return *(_private);
}

}