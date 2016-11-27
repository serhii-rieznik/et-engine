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
class DX12Texture : public Texture
{
public:
	ET_DECLARE_POINTER(DX12Texture);

public:
	DX12Texture(const Description&, const BinaryDataStorage&);

	void setImageData(const BinaryDataStorage&) override;
	void updateRegion(const vec2i& pos, const vec2i& size, const BinaryDataStorage&) override;
};
}
