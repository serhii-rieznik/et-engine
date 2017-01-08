/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <string>
#include <et/rendering/interface/pipelinestate.h>

namespace et
{

void initGlslangResources();
void cleanupGlslangResources();

bool hlslToSPIRV(const std::string& source, std::vector<uint32_t>& vertexBin, std::vector<uint32_t>& fragmentBin, 
	Program::Reflection& reflection);

}
