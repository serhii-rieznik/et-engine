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

class CubemapProcessor
{
public:
	void wrapEquirectangularTextureToCubemap(RenderInterface::Pointer, const Texture::Pointer& input, const Texture::Pointer& output);

private:
	Camera::Pointer _camera = Camera::Pointer::create();
};

}
