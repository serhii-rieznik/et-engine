/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/scene3d.h>

namespace et
{
	class RenderContext;
	class ModelLoader
	{
	public:
		virtual s3d::ElementContainer::Pointer load(RenderContext*, s3d::Storage&, ObjectsCache&) = 0;

	public:
		virtual ~ModelLoader() { }
	};
}