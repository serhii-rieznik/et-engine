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

	struct Description
	{
		TextureFiltration minFilter = TextureFiltration::Linear;
		TextureFiltration magFilter = TextureFiltration::Linear;
		TextureFiltration mipFilter = TextureFiltration::Linear;
		TextureWrap wrapU = TextureWrap::Repeat;
		TextureWrap wrapV = TextureWrap::Repeat;
		TextureWrap wrapW = TextureWrap::Repeat;
		CompareFunction compareFunction = CompareFunction::Always;
		float maxAnisotropy = 16.0f;
		float minLod = 0.0f;
		float maxLod = std::numeric_limits<float>::max();
		bool compareEnabled = false;
	};
};
}
