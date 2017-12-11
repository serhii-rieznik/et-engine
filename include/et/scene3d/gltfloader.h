/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/modelloader.h>

namespace et {

class GLTFLoaderPrivate;
class GLTFLoader : public ModelLoader
{
public:
	GLTFLoader(const std::string& fileName);
	~GLTFLoader();

	s3d::ElementContainer::Pointer load(RenderInterface::Pointer, s3d::Storage&, ObjectsCache&) override;

private:
	ET_DECLARE_PIMPL(GLTFLoader, 256);
};

}
