/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/rendering/interface/texture.h>

namespace et {
class TextureDescription : public LoadableObject, public Texture::Description
{
public:
	ET_DECLARE_POINTER(TextureDescription);

public:
	BinaryDataStorage data;

	bool load(const std::string& name);
	bool preload(const std::string& name, bool fillWithZero);

	const Texture::Description& desc() const { return *this; }

public:
	TextureDescription() = default;
	TextureDescription(const Texture::Description& r) : Texture::Description(r) {}
	TextureDescription(const std::string& fileName) { load(fileName); }

	bool valid() const {
		return (size.square() > 0);
	}
};

using TextureDescriptionUpdateMethod = void(TextureDescription::Pointer);
inline void nullTextureDescriptionUpdateMethod(TextureDescription::Pointer) {}

}
