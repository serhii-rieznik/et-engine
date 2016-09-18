/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan.h>
#include <fstream>

namespace et
{

class VulkanProgramPrivate
{
public:
	VulkanProgramPrivate(VulkanState& v)
		: vulkan(v)
	{
	}

	VulkanState& vulkan;
	VulkanShaderModules modules;
};

VulkanProgram::VulkanProgram(VulkanState& v)
{
	ET_PIMPL_INIT(VulkanProgram, v);
}

VulkanProgram::~VulkanProgram()
{
	ET_PIMPL_FINALIZE(VulkanProgram)
}

void VulkanProgram::build(const std::string& vertexSource, const std::string& fragmentSource)
{
	Vector<char> code;
	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	{
		std::ifstream fIn(vertexSource.c_str(), std::ios::in | std::ios::binary);
		code.resize(streamSize(fIn));
		fIn.read(code.data(), code.size());
		fIn.close();
		ET_ASSERT(code.size() % 4 == 0);
	}
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	createInfo.codeSize = code.size();
	VULKAN_CALL(vkCreateShaderModule(_private->vulkan.device, &createInfo, nullptr, &_private->modules.vertex));
	_private->modules.stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	_private->modules.stageCreateInfo[0].module = _private->modules.vertex;
	_private->modules.stageCreateInfo[0].pName = "main";
	
	{
		std::ifstream fIn(fragmentSource.c_str(), std::ios::in | std::ios::binary);
		code.resize(streamSize(fIn));
		fIn.read(code.data(), code.size());
		fIn.close();
		ET_ASSERT(code.size() % 4 == 0);
	}
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	createInfo.codeSize = code.size();
	VULKAN_CALL(vkCreateShaderModule(_private->vulkan.device, &createInfo, nullptr, &_private->modules.fragment));
	_private->modules.stageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	_private->modules.stageCreateInfo[1].module = _private->modules.fragment;
	_private->modules.stageCreateInfo[1].pName = "main";}

const VulkanShaderModules& VulkanProgram::shaderModules() const
{
	return _private->modules;
}

void VulkanProgram::setTransformMatrix(const mat4 & m, bool force)
{
}

void VulkanProgram::setCameraProperties(const Camera & cam)
{
}

void VulkanProgram::setDefaultLightPosition(const vec3 & p, bool force)
{
}

}
