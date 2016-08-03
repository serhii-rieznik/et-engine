/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal_program.h>
#include <et/rendering/metal/metal.h>

namespace et
{
class MetalProgramPrivate
{
};

MetalProgram::MetalProgram(MetalState&)
{
    
}

MetalProgram::~MetalProgram()
{
}
    
void MetalProgram::bind()
{
}

void MetalProgram::build(const std::string& vertexSource, const std::string& fragmentSource)
{
}

void MetalProgram::setTransformMatrix(const mat4 &m, bool force)
{
}

void MetalProgram::setCameraProperties(const Camera& cam)
{
}

void MetalProgram::setDefaultLightPosition(const vec3& p, bool force)
{
}
    
}
