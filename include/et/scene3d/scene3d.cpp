/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/scene3d.h>
#include <et/scene3d/serialization.h>

using namespace et;
using namespace s3d;

Scene::Scene(const std::string& name) :
	ElementContainer(name, nullptr)
{

}

VertexStorage::Pointer Scene::vertexStorageWithName(const std::string& name)
{
	const auto& all = _storage.vertexStorages();
	for (const auto& vs : all)
	{
		if (vs->name() == name)
			return vs;
	}
	
	return VertexStorage::Pointer();
}

IndexArray::Pointer Scene::indexArrayWithName(const std::string&)
{
	return _storage.indexArray();
}

VertexStream::Pointer Scene::vertexStreamWithStorageName(const std::string& name)
{
	for (const auto& vao : _storage.vertexStreams())
	{
		if (vao->vertexBuffer()->name() == name)
			return vao;
	}
	
	return VertexStream::Object::Pointer();
}

BaseElement::Pointer Scene::createElementOfType(ElementType type, BaseElement* parent)
{
	switch (type)
	{
		case ElementType::Container:
			return ElementContainer::Pointer::create(emptyString, parent);
			
		case ElementType::Mesh:
			return Mesh::Pointer::create(emptyString, parent);
						
		default:
		{
			log::warning("[s3d::Scene] Attempt to create element with invalid type: %u", static_cast<uint32_t>(type));
			return ElementContainer::Pointer::create(emptyString, parent);
		}
	}
}

void Scene::cleanupGeometry()
{
	auto meshes = childrenOfType(ElementType::Mesh);
	for (s3d::BaseElement::Pointer& mesh : meshes)
        mesh->setParent(nullptr);

	_storage.flush();
}
