/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>

namespace et
{
class Sampler : public Object
{
public:
	ET_DECLARE_POINTER(Sampler);

	struct SamplingParameters
	{
		TextureFiltration minFilter = TextureFiltration::Linear;
		TextureFiltration magFilter = TextureFiltration::Linear;
		TextureFiltration mipFilter = TextureFiltration::Linear;
		TextureWrap wrapU = TextureWrap::Repeat;
		TextureWrap wrapV = TextureWrap::Repeat;
		TextureWrap wrapW = TextureWrap::Repeat;
		float maxAnisotropy = 1.0f;
	};

	union Description
	{
		SamplingParameters params;
		uint64_t hash;

		Description() :
			params() { }
	};
};
}
