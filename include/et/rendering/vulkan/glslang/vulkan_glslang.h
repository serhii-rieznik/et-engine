/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <string>
#include <et/rendering/base/rendering.h>

namespace et
{

void initGlslangResources();
void cleanupGlslangResources();

using SPIRSource = std::vector<uint32_t>;
using SPIRProgramStageMap = std::map<ProgramStage, SPIRSource>;

bool hlslToSPIRV(const std::string& source, std::vector<uint32_t>& vertexBin, std::vector<uint32_t>& fragmentBin, Program::Reflection& reflection);
bool generateSPIRFromHLSL(const std::string& source, SPIRProgramStageMap& stages, Program::Reflection& reflection);

}
