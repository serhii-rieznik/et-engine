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

	struct ReflectionSet
	{
		ProgramStage stage = ProgramStage::max;
		Vector<std::pair<std::string, uint32_t>> textures;
		Vector<std::pair<std::string, uint32_t>> samplers;
		Vector<std::pair<std::string, uint32_t>> images;
	};
	using Reflection = Vector<ReflectionSet>;

	struct TextureBinding
	{
		Texture::Pointer image;
		ResourceRange range;
	};

	struct DescriptionSet
	{		
		Map<uint32_t, TextureBinding> textures;
		Map<uint32_t, Texture::Pointer> images;
		Map<uint32_t, Sampler::Pointer> samplers;
	};
	using Description = Map<ProgramStage, DescriptionSet>;

public:
	TextureSet() = default;
	virtual ~TextureSet() = default;
};

}
