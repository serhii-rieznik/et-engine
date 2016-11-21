/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/raytraceobjects.h>

namespace et
{
namespace rt
{

class ReconstructionFilter : public Shared
{
public:
	virtual float weight(const vec2& sample) = 0;
};

class BoxFilter : public ReconstructionFilter
{
public:
	float weight(const vec2& sample) override;
};

class TriangleFilter : public ReconstructionFilter
{
public:
	float weight(const vec2& sample) override;
};

}

}
