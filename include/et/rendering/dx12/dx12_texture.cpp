/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_texture.h>

namespace et
{
DX12Texture::DX12Texture(TextureDescription::Pointer desc) 
	: Texture(desc)
{
}

void DX12Texture::setImageData(const BinaryDataStorage&)
{
}

void DX12Texture::updateRegion(const vec2i & pos, const vec2i & size, const BinaryDataStorage &)
{
}

}
