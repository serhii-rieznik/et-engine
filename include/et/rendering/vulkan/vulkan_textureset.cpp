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

static struct VulkanTextureSetStats
{
	uint32_t setsAllocated = 0;
	uint32_t setsReleased = 0;
	uint32_t descsAllocated = 0;
	uint32_t descsReleased = 0;
} _stats;

 VulkanTextureSet::VulkanTextureSet(VulkanRenderer* renderer, VulkanState& vulkan, const Description& desc) 
{
	ET_PIMPL_INIT(VulkanTextureSet, vulkan);

	size_t objectCount = 0;
	for (const auto& entry : desc)
	{
		objectCount += entry.second.textures.size();
		objectCount += entry.second.samplers.size();
		objectCount += entry.second.images.size();
	}

	Vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(objectCount);
	
	Vector<VkWriteDescriptorSet> writeSet;
	writeSet.reserve(objectCount);
	
	Vector<VkDescriptorImageInfo> imageInfos;
	imageInfos.reserve(objectCount);

	for (const auto& entry : desc)
	{
		VkShaderStageFlagBits stageFlags = vulkan::programStageValue(entry.first);

		for (const auto& e : entry.second.textures)
		{
				bindings.emplace_back();
				VkDescriptorSetLayoutBinding& binding = bindings.back();
				binding.binding = static_cast<uint32_t>(e.first);
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				binding.stageFlags = stageFlags;

				imageInfos.emplace_back();
				VkDescriptorImageInfo& info = imageInfos.back();
				info.imageView = VulkanTexture::Pointer(e.second.image)->nativeTexture().imageView(e.second.range);
				info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				writeSet.emplace_back();
				VkWriteDescriptorSet& ws = writeSet.back();
				ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				ws.descriptorCount = binding.descriptorCount;
				ws.descriptorType = binding.descriptorType;
				ws.dstBinding = binding.binding;
				ws.pImageInfo = imageInfos.data() + imageInfos.size() - 1;
		}
		
		for (const auto& e : entry.second.samplers)
		{
				bindings.emplace_back();
				VkDescriptorSetLayoutBinding& binding = bindings.back();
				binding.binding = MaterialSamplerBindingOffset + static_cast<uint32_t>(e.first);
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				binding.stageFlags = stageFlags;

				imageInfos.emplace_back();
				VkDescriptorImageInfo& info = imageInfos.back();
				info.sampler = VulkanSampler::Pointer(e.second)->nativeSampler().sampler;

				writeSet.emplace_back();
				VkWriteDescriptorSet& ws = writeSet.back();
				ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				ws.descriptorCount = binding.descriptorCount;
				ws.descriptorType = binding.descriptorType;
				ws.dstBinding = binding.binding;
				ws.pImageInfo = imageInfos.data() + imageInfos.size() - 1;
		}

		for (const auto& e : entry.second.images)
		{
				bindings.emplace_back();
				VkDescriptorSetLayoutBinding& binding = bindings.back();
				binding.binding = static_cast<uint32_t>(e.first);
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.stageFlags = stageFlags;

				imageInfos.emplace_back();
				VkDescriptorImageInfo& info = imageInfos.back();
				info.imageView = VulkanTexture::Pointer(e.second)->nativeTexture().imageView(ResourceRange::whole);
				info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				writeSet.emplace_back();
				VkWriteDescriptorSet& ws = writeSet.back();
				ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				ws.descriptorCount = binding.descriptorCount;
				ws.descriptorType = binding.descriptorType;
				ws.dstBinding = binding.binding;
				ws.pImageInfo = imageInfos.data() + imageInfos.size() - 1;
		}
	}

	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &createInfo, nullptr, &_private->descriptorSetLayout));
	_stats.descsAllocated++;

	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = vulkan.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_private->descriptorSetLayout;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &allocInfo, &_private->descriptorSet));
	_stats.setsAllocated++;

	for (auto& ws : writeSet)
		ws.dstSet = _private->descriptorSet;

	vkUpdateDescriptorSets(vulkan.device, static_cast<uint32_t>(writeSet.size()), writeSet.data(), 0, nullptr);
}

VulkanTextureSet::~VulkanTextureSet()
{
	VULKAN_CALL(vkFreeDescriptorSets(_private->vulkan.device, _private->vulkan.descriptorPool, 1, &_private->descriptorSet));
	_stats.setsReleased++;

	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->descriptorSetLayout, nullptr);
	_stats.descsReleased++;

	ET_PIMPL_FINALIZE(VulkanTextureSet);
}

const VulkanNativeTextureSet& VulkanTextureSet::nativeSet() const
{
	return *(_private);
}

}