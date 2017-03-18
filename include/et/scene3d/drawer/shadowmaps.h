/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderer.h>

namespace et
{
namespace s3d
{

class ShadowmapProcessor : public Shared
{
public:
	ET_DECLARE_POINTER(ShadowmapProcessor);

public:
	const Texture::Pointer& directionalShadowmap() const;

private:
	Texture::Pointer _directionalShadowmap;
};

inline const Texture::Pointer& ShadowmapProcessor::directionalShadowmap() const {
	return _directionalShadowmap;
}

}
}
