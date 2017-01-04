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

	Vector<VkDescriptorSetLayoutBinding> bindings;
	Vector<VkWriteDescriptorSet> writeSet;
	Vector<VkDescriptorImageInfo> imageInfos;
	bindings.reserve(8);
	writeSet.reserve(8);
	imageInfos.reserve(8);

	using TSPair = std::pair<Texture::Pointer, Sampler::Pointer>;
	Map<uint32_t, TSPair> mergedData;
	
	for (const auto& b : desc.vertexTextures)
		mergedData[b.first].first = b.second;
	for (const auto& b : desc.fragmentTextures)
		mergedData[b.first].first = b.second;
	for (const auto& b : desc.vertexSamplers)
		mergedData[b.first].second = b.second;
	for (const auto& b : desc.fragmentSamplers)
		mergedData[b.first].second = b.second;

	for (const auto& b : mergedData)
	{
		bindings.emplace_back();
		imageInfos.emplace_back();
		writeSet.emplace_back();

		bool isTextureBinding = b.second.first.valid();

		VkDescriptorSetLayoutBinding& binding = bindings.back();
		binding.binding = b.first;
		binding.descriptorCount = 1;
		binding.descriptorType = isTextureBinding ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLER;
		binding.stageFlags = VK_SHADER_STAGE_ALL;

		VkDescriptorImageInfo& info = imageInfos.back();
		if (isTextureBinding)
		{
			info.imageView = VulkanTexture::Pointer(b.second.first)->nativeTexture().imageView;
			info.imageLayout = VulkanTexture::Pointer(b.second.first)->nativeTexture().layout;
		}
		else 
		{
			info.sampler = VulkanSampler::Pointer(b.second.second)->nativeSampler().sampler;
		}

		VkWriteDescriptorSet& ws = writeSet.back();
		ws.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ws.descriptorCount = binding.descriptorCount;
		ws.descriptorType = binding.descriptorType;
		ws.dstBinding = binding.binding;
		ws.pImageInfo = imageInfos.data() + imageInfos.size() - 1;
	}

	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &createInfo, nullptr, &_private->descriptorSetLayout));

	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = vulkan.descriptprPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_private->descriptorSetLayout;
	VULKAN_CALL(vkAllocateDescriptorSets(vulkan.device, &allocInfo, &_private->descriptorSet));

	for (auto& ws : writeSet)
		ws.dstSet = _private->descriptorSet;

	vkUpdateDescriptorSets(vulkan.device, static_cast<uint32_t>(writeSet.size()), writeSet.data(), 0, nullptr);
}

VulkanTextureSet::~VulkanTextureSet()
{
	ET_PIMPL_FINALIZE(VulkanTextureSet);
}

const VulkanNativeTextureSet& VulkanTextureSet::nativeSet() const
{
	return *(_private);
}

}