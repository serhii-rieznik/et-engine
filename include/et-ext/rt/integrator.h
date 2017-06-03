/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/rtscene.h>

namespace et
{
namespace rt
{

using EvaluateFunction = float4(*)(const Scene&, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength);

float4 evaluateNormals(const Scene&, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength);
float4 evaluateAmbientOcclusion(const Scene&, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength);
float4 evaluateGlobalIllumination(const Scene&, const Ray& inRay, uint32_t maxPathLength, uint32_t& pathLength);

}
}
