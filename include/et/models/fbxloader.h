/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
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