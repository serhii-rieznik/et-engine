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
class VulkanTexturePrivate;
class VulkanTexture : public Texture
{
public:
	ET_DECLARE_POINTER(VulkanTexture);

public:
	VulkanTexture(VulkanState&, TextureDescription::Pointer);
	~VulkanTexture();

	void setImageData(const BinaryDataStorage&) override;

private:
	ET_DECLARE_PIMPL(VulkanTexture, 64)
};
}
