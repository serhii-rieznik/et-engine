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

IndexArray::Pointer Scene::indexArrayWithName(const std::string& name)
{
	return _storage.indexArray();
}

VertexArrayObject Scene::vertexArrayWithStorageName(const std::string& name)
{
	for (const auto& vao : _vertexArrays)
	{
		if (vao->vertexBuffer()->name() == name)
			return vao;
	}
	
	return VertexArrayObject();
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

SceneMaterial* Scene::materialWithName(const std::string& mName)
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

void Scene::deserializeWithOptions(et::RenderContext* rc, Dictionary info, const std::string& basePath,
	ObjectsCache& cache, uint32_t options)
{
	_serializationBasePath = basePath;
	_storage.deserializeWithOptions(rc,  info.dictionaryForKey(kStorage), this, cache, options);
	
	if (options & DeserializeOption_CreateVertexBuffers)
	{
		buildVertexBuffers(rc);
	}
	
	ElementContainer::deserialize(info, this);
	
	if ((options & DeserializeOption_KeepGeometry) == 0)
	{
		cleanupGeometry();
	}
	
	if ((options & DeserializeOption_KeepSupportMeshes) == 0)
	{
		cleanUpSupportMehses();
	}
}

void Scene::buildVertexBuffers(et::RenderContext* rc)
{
	for (auto vs : _storage.vertexStorages())
	{
		std::string vaoName = "vao-" + intToStr(_vertexArrays.size() + 1);
		auto vao = rc->vertexBufferFactory().createVertexArrayObject(vaoName);
		
		if (_mainIndexBuffer.invalid())
		{
			_mainIndexBuffer = rc->vertexBufferFactory().createIndexBuffer("mainIndexBuffer",
				_storage.indexArray(), BufferDrawType::Static);
		}
		
		auto vb = rc->vertexBufferFactory().createVertexBuffer(vs->name(), vs, BufferDrawType::Static);
		vao->setBuffers(vb, _mainIndexBuffer);
		
		_vertexArrays.push_back(vao);
	}
}

void Scene::cleanupGeometry()
{
	auto meshes = childrenOfType(ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		mesh->setVertexStorage(VertexStorage::Pointer());
		mesh->setIndexArray(IndexArray::Pointer());
	}
	meshes = childrenOfType(ElementType::SupportMesh);
	for (s3d::SupportMesh::Pointer mesh : meshes)
	{
		mesh->setVertexStorage(VertexStorage::Pointer());
		mesh->setIndexArray(IndexArray::Pointer());
	}
	_storage.flush();
}

void Scene::cleanUpSupportMehses()
{
	auto meshes = childrenOfType(ElementType::SupportMesh);
	for (s3d::SupportMesh::Pointer mesh : meshes)
	{
		mesh->setParent(nullptr);
	}
}
