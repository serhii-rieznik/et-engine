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

struct Evaluate
{
	uint32_t rayIndex = 0;
	uint32_t totalRayCount = 0;
	uint32_t maxPathLength = 0;
	uint32_t pathLength = 0;
};

using EvaluateFunction = float4(*)(Scene&, const Ray&, Evaluate&);

float4 evaluateNormals(Scene&, const Ray& inRay, Evaluate&);
float4 evaluateAmbientOcclusion(Scene&, const Ray& inRay, Evaluate&);
float4 evaluateGlobalIllumination(Scene&, const Ray& inRay, Evaluate&);

}
}
