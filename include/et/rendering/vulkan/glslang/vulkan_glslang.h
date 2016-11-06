/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <string>

namespace et
{
bool glslToSPIRV(const std::string& vertexSource, const std::string& fragmentSource, 
	std::vector<uint32_t>& vertexBin, std::vector<uint32_t>& fragmentBin);
}
