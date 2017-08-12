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
		UnorderedMap<std::string, uint32_t> textures;
		UnorderedMap<std::string, uint32_t> samplers;
		UnorderedMap<std::string, uint32_t> images;
	};
	
	struct TextureBinding
	{
		Texture::Pointer image;
		ResourceRange range;
	};

	struct DescriptionSet
	{		
		Map<MaterialTexture, TextureBinding> textures;
		Map<MaterialTexture, Sampler::Pointer> samplers;
		Map<StorageBuffer, Texture::Pointer> images;
	};

	using Reflection = Map<ProgramStage, ReflectionSet>;
	using Description = Map<ProgramStage, DescriptionSet>;

public:
	TextureSet() = default;
	virtual ~TextureSet() = default;
};

}
