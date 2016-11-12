/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/camera/camera.h>
#include <et/rendering/interface/databuffer.h>

namespace et
{

class SharedConstBufferPrivate;
class SharedConstBuffer
{
public:
	void init(RenderInterface*);
	void shutdown();

private:
	ET_DECLARE_PIMPL(SharedConstBuffer, 128);
};

}
