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
class VulkanNativeTexture;
class VulkanTexturePrivate;
class VulkanTexture : public Texture
{
public:
	ET_DECLARE_POINTER(VulkanTexture);

public:
	VulkanTexture(VulkanState&, const Description&, const BinaryDataStorage&);
	~VulkanTexture();

	void setImageData(const BinaryDataStorage&) override;
	void updateRegion(const vec2i& pos, const vec2i& size, const BinaryDataStorage&);

	const VulkanNativeTexture& nativeTexture() const;

private:
	ET_DECLARE_PIMPL(VulkanTexture, 384);
};
}
