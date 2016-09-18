/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_program.h>

namespace et
{

void DX12Program::build(const std::string & vertexSource, const std::string & fragmentSource)
{
}

void DX12Program::setTransformMatrix(const mat4 & m, bool force)
{
}

void DX12Program::setCameraProperties(const Camera & cam)
{
}

void DX12Program::setDefaultLightPosition(const vec3 & p, bool force)
{
}

}
