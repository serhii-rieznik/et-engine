/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/scene3d.h>
#include <et/rendering/interface/renderer.h>

namespace et
{
	class ModelLoader
	{
	public:
		virtual s3d::ElementContainer::Pointer load(RenderInterface::Pointer, s3d::Storage&, ObjectsCache&) = 0;

	public:
		virtual ~ModelLoader() { }
	};
}
