/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/shadersource.h>
#include <et/rendering/vulkan/glslang/vulkan_glslang.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan.h>
#include <fstream>

namespace et
{

class VulkanProgramPrivate : public VulkanShaderModules
{
public:
	VulkanProgramPrivate(VulkanState& v)
		: vulkan(v) { }

	VulkanState& vulkan;
};

VulkanProgram::VulkanProgram(VulkanState& v)
{
	ET_PIMPL_INIT(VulkanProgram, v);
}

VulkanProgram::~VulkanProgram()
{
	vkDestroyShaderModule(_private->vulkan.device, _private->vertex, nullptr);
	vkDestroyShaderModule(_private->vulkan.device, _private->fragment, nullptr);
	ET_PIMPL_FINALIZE(VulkanProgram)
}

void VulkanProgram::build(const std::string& source)
{
	std::string vertexSource = source;
	parseShaderSource(vertexSource, emptyString, StringList(), [](ParseDirective dir, std::string& code, uint32_t pos) 
	{
		if (dir == ParseDirective::StageDefine) {
			code.insert(pos, "#define ET_VERTEX_SHADER 1");
		}
	}, { });

	std::string fragmentSource = source;
	parseShaderSource(fragmentSource, emptyString, StringList(), [](ParseDirective dir, std::string& code, uint32_t pos)
	{
		if (dir == ParseDirective::StageDefine) {
			code.insert(pos, "#define ET_FRAGMENT_SHADER 1");
		}
	}, { });

	std::vector<uint32_t> vertexBin;
	std::vector<uint32_t> fragmentBin;
	if (glslToSPIRV(vertexSource, fragmentSource, vertexBin, fragmentBin, _reflection))
	{
		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

		createInfo.pCode = vertexBin.data();
		createInfo.codeSize = vertexBin.size() * sizeof(uint32_t);
		VULKAN_CALL(vkCreateShaderModule(_private->vulkan.device, &createInfo, nullptr, &_private->vertex));

		createInfo.pCode = fragmentBin.data();
		createInfo.codeSize = fragmentBin.size() * sizeof(uint32_t);
		VULKAN_CALL(vkCreateShaderModule(_private->vulkan.device, &createInfo, nullptr, &_private->fragment));

		_private->stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		_private->stageCreateInfo[0].module = _private->vertex;
		_private->stageCreateInfo[0].pName = "main";
		_private->stageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		_private->stageCreateInfo[1].module = _private->fragment;
		_private->stageCreateInfo[1].pName = "main";
	}
}

const VulkanShaderModules& VulkanProgram::shaderModules() const
{
	return *(_private);
}

}
