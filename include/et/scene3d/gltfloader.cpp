/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/gltfloader.h>
#include <et/core/json.h>

namespace et {

class GLTFLoaderPrivate
{
public:
	std::string inputFileName;
	Dictionary data;
};

GLTFLoader::GLTFLoader(const std::string& fileName) {
	ET_PIMPL_INIT(GLTFLoader);
}

GLTFLoader::~GLTFLoader() {
	ET_PIMPL_FINALIZE(GLTFLoader);
}

s3d::ElementContainer::Pointer GLTFLoader::load(RenderInterface::Pointer, s3d::Storage&, ObjectsCache&) {
	s3d::ElementContainer::Pointer result(PointerInit::CreateInplace);

	VariantClass cls = VariantClass::Invalid;
	json::deserialize(loadTextFile(_private->inputFileName), cls);
	if (cls != VariantClass::Dictionary)
	{
		log::error("Failed to load GLTF from file: %s", _private->inputFileName.c_str());
		return result;
	}

	return result;
}

}