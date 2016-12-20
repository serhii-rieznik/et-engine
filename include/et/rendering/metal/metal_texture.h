/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/texture.h>

namespace et
{
struct MetalState;
struct MetalNativeTexture;
class MetalTexturePrivate;
class MetalTexture : public Texture
{
public:
	ET_DECLARE_POINTER(MetalTexture);
	
public:
	MetalTexture() = default;
	MetalTexture(MetalState&, TextureDescription::Pointer);
	~MetalTexture();
	
	const MetalNativeTexture& nativeTexture() const;
	
	void setImageData(const BinaryDataStorage&) override;
	void updateRegion(const vec2i& pos, const vec2i& size, const BinaryDataStorage&) override;

private:
	ET_DECLARE_PIMPL(MetalTexture, 64);
};
}
