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
	class BSDF
	{
	public:
		float evaluate(const rt::float4& Wi, const rt::float4& Wo, const rt::float4& n);
	};
}
}
