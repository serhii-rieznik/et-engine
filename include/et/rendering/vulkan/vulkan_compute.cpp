/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_compute.h>
#include <et/rendering/vulkan/vulkan.h>
#include <et/rendering/vulkan/vulkan_program.h>

namespace et
{

class VulkanComputePrivate : public VulkanNativeCompute
{
public:
	VulkanComputePrivate(VulkanState& v) :
		vulkan(v)
	{
	}

	VulkanState& vulkan;
};

VulkanCompute::VulkanCompute(VulkanState& vulkan, const Material::Pointer& mat)
{
	ET_PIMPL_INIT(VulkanCompute, vulkan);

	VulkanProgram::Pointer program = mat->configuration(kCompute).program;

	VkDescriptorSetLayoutCreateInfo layoutSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutSetInfo.bindingCount = 0;
	layoutSetInfo.pBindings = nullptr;
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &layoutSetInfo, nullptr, &_private->texturesSetLayout));

	VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreateInfo.pSetLayouts = &_private->texturesSetLayout;
	layoutCreateInfo.setLayoutCount = 1;
	VULKAN_CALL(vkCreatePipelineLayout(vulkan.device, &layoutCreateInfo, nullptr, &_private->pipelineLayout));
	
	VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	createInfo.stage = program->shaderModules().stageCreateInfos.begin()->second;
	createInfo.layout = _private->pipelineLayout;
	VULKAN_CALL(vkCreateComputePipelines(vulkan.device, vulkan.pipelineCache, 1, &createInfo, nullptr, &_private->pipeline));
}

VulkanCompute::~VulkanCompute()
{
	vkDestroyPipeline(_private->vulkan.device, _private->pipeline, nullptr);
	vkDestroyPipelineLayout(_private->vulkan.device, _private->pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(_private->vulkan.device, _private->texturesSetLayout, nullptr);
	ET_PIMPL_FINALIZE(VulkanCompute);
}

const VulkanNativeCompute& VulkanCompute::nativeCompute() const
{
	return (*_private);
}

}
