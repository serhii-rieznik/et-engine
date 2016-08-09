/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_program.h>

namespace et
{

void VulkanProgram::bind()
{
}

void VulkanProgram::build(const std::string & vertexSource, const std::string & fragmentSource)
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
