/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/sampler.h>
#include <et/rendering/interface/texture.h>

namespace et
{

class TextureSet : public Object
{
public:
	ET_DECLARE_POINTER(TextureSet);

	struct Reflection
	{
		UnorderedMap<std::string, uint32_t> vertexTextures;
		UnorderedMap<std::string, uint32_t> vertexSamplers;
		UnorderedMap<std::string, uint32_t> fragmentTextures;
		UnorderedMap<std::string, uint32_t> fragmentSamplers;
	};

	struct Description
	{
		Map<uint32_t, Texture::Pointer> vertexTextures;
		Map<uint32_t, Sampler::Pointer> vertexSamplers;
		Map<uint32_t, Texture::Pointer> fragmentTextures;
		Map<uint32_t, Sampler::Pointer> fragmentSamplers;
	};

public:
	TextureSet() = default;
	virtual ~TextureSet() = default;
};

}
