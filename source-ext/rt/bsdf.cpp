/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/bsdf.h>

float et::rt::BSDF::evaluate(const rt::float4& Wi, const rt::float4& Wo, const rt::float4& n)
{
	return (1.0f / PI);
}
