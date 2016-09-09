/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanProgramPrivate
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
	ET_PIMPL_FINALIZE(VulkanProgram)
}

void VulkanProgram::bind()
{
}

void VulkanProgram::build(const std::string & source, const std::string & fragmentSource)
{
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
