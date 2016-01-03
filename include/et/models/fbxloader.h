/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/scene3d.h>
#include <et/models/modelloader.h>

namespace et
{
	class RenderContext;
	class FBXLoader : public ModelLoader
	{
	public:
		FBXLoader(const std::string& filename);

		void setShouldCreateRenderObjects(bool);

		s3d::ElementContainer::Pointer load(RenderContext*, s3d::Storage&, ObjectsCache&) override;

	private:
		std::string _filename;
		bool _shouldCreateRenderObjects = true;
	};
}