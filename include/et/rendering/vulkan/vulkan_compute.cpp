/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_compute.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanComputePrivate : public VulkanNativePipeline
{
public:
	VulkanComputePrivate(VulkanState& v) :
		vulkan(v)
	{
	}

	VulkanState& vulkan;
};

VulkanCompute::VulkanCompute(VulkanState& vulkan, const Material::Pointer& mat) :
	Compute(mat)
{
	ET_PIMPL_INIT(VulkanCompute, vulkan);
}

VulkanCompute::~VulkanCompute()
{
	_private->cleanup(_private->vulkan);
	ET_PIMPL_FINALIZE(VulkanCompute);
}

void VulkanCompute::build(const VulkanRenderPass::Pointer& pass)
{
	if (_private->pipeline != nullptr)
		return;

	VulkanProgram::Pointer program = material()->configuration(pass->info().name).program;
	_private->buildLayout(_private->vulkan, program->reflection(), pass->nativeRenderPass().dynamicDescriptorSetLayout);

	VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	createInfo.stage = program->shaderModules().stageCreateInfos.begin()->second;
	createInfo.layout = _private->layout;
	VULKAN_CALL(vkCreateComputePipelines(_private->vulkan.device, _private->vulkan.pipelineCache, 1, &createInfo, nullptr, &_private->pipeline));
}

const VulkanNativePipeline& VulkanCompute::nativeCompute() const
{
	return (*_private);
}

}
