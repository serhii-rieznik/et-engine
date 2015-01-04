/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/scene3d.h>

namespace et
{
	class RenderContext;
	class FBXLoader
	{
	public:
		FBXLoader(const std::string& filename);

		s3d::ElementContainer::Pointer load(RenderContext* rc, ObjectsCache& textureCache);

	private:
		std::string _filename;
	};
}