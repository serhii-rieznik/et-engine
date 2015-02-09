/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
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

BaseElement::Pointer Scene::createElementOfType(uint64_t type, BaseElement* parent)
{
	switch (type)
	{
	case ElementType_Container:
		return ElementContainer::Pointer::create(emptyString, parent);
	case ElementType_Mesh:
		return Mesh::Pointer::create(emptyString, parent);
	case ElementType_SupportMesh:
		return SupportMesh::Pointer::create(emptyString, parent);
	case ElementType_Storage:
		return Storage::Pointer::create(emptyString, parent);
	case ElementType_Camera:
		return CameraElement::Pointer::create(emptyString, parent);
	default:
		return ElementContainer::Pointer::create(emptyString, parent);
	}
}

IndexArray::Pointer Scene::primaryIndexArray()
{
	BaseElement::List storages = childrenOfType(ElementType_Storage);
	ET_ASSERT(!storages.empty())
	return Storage::Pointer(storages.front())->indexArray();
}

VertexStorage::Pointer Scene::vertexStorageWithName(const std::string& vsName)
{
	BaseElement::List storages = childrenOfType(ElementType_Storage);
	for (Storage::Pointer storage : storages)
	{
		for (const auto& vs : storage->vertexStorages())
		{
			if (vs->name() == vsName)
				return vs;
		}
	}
	return VertexStorage::Pointer();
}

Material::Pointer Scene::materialWithName(const std::string& mName)
{
	BaseElement::List storages = childrenOfType(ElementType_Storage);
	for (Storage::Pointer storage : storages)
	{
		const auto& materials = storage->materials();

		if (materials.count(mName) > 0)
			return materials.at(mName);

		for (const auto& kv : materials)
		{
			if (kv.second->name() == mName)
				return kv.second;
		}
	}
	
	return Material::Pointer();
}

/*
 * Serialization stuff
 */
Dictionary Scene::serialize(const std::string& basePath)
{
	Dictionary result;
	ElementContainer::serialize(result, basePath);
	return result;
}
