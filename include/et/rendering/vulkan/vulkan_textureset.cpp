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
	std::array<VkDescriptorSetLayoutBinding, MaxObjectsCount> bindings = {};
	std::array<VkWriteDescriptorSet, MaxObjectsCount> writeSet = {};
	std::array<VkDescriptorImageInfo, MaxObjectsCount> imageInfos = {};

	uint32_t objectsCount = 0;
	for (const auto& entry : desc)
	{
		VkShaderStageFlagBits stageFlags = vulkan::programStageValue(entry.first);

		for (const auto& e : entry.second.textures)
		{
			VkDescriptorSetLayoutBinding& binding = bindings[objectsCount];
			binding.binding = static_cast<uint32_t>(e.first);
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			binding.stageFlags = stageFlags;

			VkDescriptorImageInfo& info = imageInfos[objectsCount];
			info.imageView = VulkanTexture::Pointer(e.second.image)->nativeTexture().imageView(e.second.range);
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet& ws = writeSet[objectsCount];
			ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ws.descriptorCount = binding.descriptorCount;
			ws.descriptorType = binding.descriptorType;
			ws.dstBinding = binding.binding;
			ws.pImageInfo = imageInfos.data() + objectsCount;

			++objectsCount;
			ET_ASSERT(objectsCount < MaxObjectsCount);
		}

		for (const auto& e : entry.second.samplers)
		{
			VkDescriptorSetLayoutBinding& binding = bindings[objectsCount];;
			binding.binding = MaterialSamplerBindingOffset + static_cast<uint32_t>(e.first);
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			binding.stageFlags = stageFlags;

			VkDescriptorImageInfo& info = imageInfos[objectsCount];
			info.sampler = VulkanSampler::Pointer(e.second)->nativeSampler().sampler;

			VkWriteDescriptorSet& ws = writeSet[objectsCount];
			ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ws.descriptorCount = binding.descriptorCount;
			ws.descriptorType = binding.descriptorType;
			ws.dstBinding = binding.binding;
			ws.pImageInfo = imageInfos.data() + objectsCount;
			
			++objectsCount;
			ET_ASSERT(objectsCount < MaxObjectsCount);
		}

		for (const auto& e : entry.second.images)
		{
			VkDescriptorSetLayoutBinding& binding = bindings[objectsCount];
			binding.binding = static_cast<uint32_t>(e.first);
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.stageFlags = stageFlags;

			VkDescriptorImageInfo& info = imageInfos[objectsCount];
			info.imageView = VulkanTexture::Pointer(e.second)->nativeTexture().imageView(ResourceRange::whole);
			info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			VkWriteDescriptorSet& ws = writeSet[objectsCount];
			ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ws.descriptorCount = binding.descriptorCount;
			ws.descriptorType = binding.descriptorType;
			ws.dstBinding = binding.binding;
			ws.pImageInfo = imageInfos.data() + objectsCount;
			
			++objectsCount;
			ET_ASSERT(objectsCount < MaxObjectsCount);
		}
	}

	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.bindingCount = objectsCount;
	createInfo.pBindings = bindings.data();
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &createInfo, nullptr, &_private->descriptorSetLayout));

	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = vulkan.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_private->descriptorSetLayout;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &allocInfo, &_private->descriptorSet));

	for (auto& ws : writeSet)
		ws.dstSet = _private->descriptorSet;

	vkUpdateDescriptorSets(vulkan.device, objectsCount, writeSet.data(), 0, nullptr);
}

VulkanTextureSet::~VulkanTextureSet()
{
	VULKAN_CALL(vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptorPool, 1, &_private->descriptorSet));
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->descriptorSetLayout, nullptr);

	ET_PIMPL_FINALIZE(VulkanTextureSet);
}

const VulkanNativeTextureSet& VulkanTextureSet::nativeSet() const
{
	return *(_private);
}

}