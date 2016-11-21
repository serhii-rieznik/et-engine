/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/reconstruction.h>

namespace et
{
namespace rt
{

float BoxFilter::weight(const vec2& sample)
{
	return 1.0f;
}

float TriangleFilter::weight(const vec2& sample)
{
	union { float f; uint32_t i; } dx = { 0.5f - sample.x };
	union { float f; uint32_t i; } dy = { 0.5f - sample.y };
	dx.i &= 0x7fffffff;
	dy.i &= 0x7fffffff;
	return (1.0f - 2.0f * dx.f) * (1.0f - 2.0f * dy.f);
}

}
}
