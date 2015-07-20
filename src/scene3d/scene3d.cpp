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

BaseElement::Pointer Scene::createElementOfType(ElementType type, BaseElement* parent)
{
	switch (type)
	{
	case ElementType::Container:
		return ElementContainer::Pointer::create(emptyString, parent);

	case ElementType::Mesh:
		return Mesh::Pointer::create(emptyString, parent);

	case ElementType::SupportMesh:
		return SupportMesh::Pointer::create(emptyString, parent);

	case ElementType::Camera:
		return CameraElement::Pointer::create(emptyString, parent);

	default:
	{
		log::warning("[s3d::Scene] Attempt to create element with invalid type: %u", static_cast<uint32_t>(type));
		return ElementContainer::Pointer::create(emptyString, parent);
	}
	}
}


Material* Scene::materialWithName(const std::string& mName)
{
	auto& materials = _storage.materials();

	if (materials.count(mName) > 0)
		return materials.at(mName).ptr();

	for (auto& kv : materials)
	{
		if (kv.second->name() == mName)
			return kv.second.ptr();
	}
	
	return nullptr;
}

/*
 * Serialization stuff
 */
Dictionary Scene::serialize(const std::string& basePath)
{
	Dictionary result;
	result.setDictionaryForKey(kStorage, _storage.serialize(basePath));
	ElementContainer::serialize(result, basePath);
	return result;
}

void Scene::deserialize(et::RenderContext* rc, Dictionary info, const std::string& basePath, ObjectsCache& cache)
{
	_serializationBasePath = basePath;

	{
		Dictionary storageDictionary = info.dictionaryForKey(kStorage);
		if (storageDictionary.empty())
			log::warning("Serialized scene contains no storage");
		_storage.deserialize(rc, storageDictionary, this, cache);

		for (auto vs : _storage.vertexStorages())
		{
			VertexArrayObject vao;
			std::string vaoName = "vao-" + intToStr(_vertexArrays.size() + 1);

			if (_mainIndexBuffer.invalid())
			{
				vao = rc->vertexBufferFactory().createVertexArrayObject(vaoName, vs, BufferDrawType::Static,
					_storage.indexArray(), BufferDrawType::Static);
				_mainIndexBuffer = vao->indexBuffer();
				_mainIndexBuffer->setName("mainIndexBuffer");
			}
			else
			{
				vao = rc->vertexBufferFactory().createVertexArrayObject(vaoName, vs, BufferDrawType::Static, 
					_mainIndexBuffer);
			}
			vao->vertexBuffer()->setName(vs->name());
			_vertexArrays.push_back(vao);
		}
	}

	ElementContainer::deserialize(info, this);
}
