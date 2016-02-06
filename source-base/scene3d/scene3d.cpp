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

VertexArrayObject::Pointer Scene::vertexArrayWithStorageName(const std::string& name)
{
	for (const auto& vao : _vertexArrayObjects)
	{
		if (vao->vertexBuffer()->name() == name)
			return vao;
	}
	
	return VertexArrayObject::Object::Pointer();
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

Material::Pointer Scene::materialWithName(const std::string& name)
{
	ET_ASSERT(_currentMaterialProvider != nullptr);
	return _currentMaterialProvider->materialWithName(name);
}

SceneMaterial::Pointer Scene::sceneMaterialWithName(const std::string& mName)
{
	auto& materials = _storage.materials();

	auto i = materials.find(mName);
	if (i != materials.end())
	{
		return i->second;
	}
	
	for (auto& kv : materials)
	{
		if (kv.second->name() == mName)
		{
			return kv.second;
		}
	}
	
	return SceneMaterial::Pointer();
}

/*
 * Serialization stuff
 */
void Scene::serialize(Dictionary stream, const std::string& basePath)
{
	stream.setDictionaryForKey(kStorage, _storage.serialize(basePath));
	ElementContainer::serialize(stream, basePath);
}

Dictionary Scene::serialize(const std::string& basePath)
{
	Dictionary result;
	serialize(result, basePath);
	return result;
}

void Scene::deserializeWithOptions(et::RenderContext* rc, MaterialProvider* mp,
	Dictionary info, const std::string& basePath, ObjectsCache& cache, uint32_t options)
{
	_currentMaterialProvider = mp;
	_serializationBasePath = basePath;
	_storage.deserializeWithOptions(rc,  info.dictionaryForKey(kStorage), this, cache, options);
	
	if (options & DeserializeOption_CreateVertexBuffers)
	{
		buildVertexBuffers(rc);
	}
	
	ElementContainer::deserialize(info, this);
	
	if ((options & DeserializeOption_KeepMeshes) == 0)
	{
		cleanupGeometry();
	}
	
	if ((options & DeserializeOption_KeepHelperMeshes) == 0)
	{
		cleanUpSupportMehses();
	}
}

void Scene::buildVertexBuffers(et::RenderContext* rc)
{
	for (auto vs : _storage.vertexStorages())
	{
		std::string vaoName = "vao-" + intToStr(_vertexArrayObjects.size() + 1);
		auto vao = rc->vertexBufferFactory().createVertexArrayObject(vaoName);
		
		if (_mainIndexBuffer.invalid())
		{
			_mainIndexBuffer = rc->vertexBufferFactory().createIndexBuffer("mainIndexBuffer",
				_storage.indexArray(), BufferDrawType::Static);
		}
		
		auto vb = rc->vertexBufferFactory().createVertexBuffer(vs->name(), vs, BufferDrawType::Static);
		vao->setBuffers(vb, _mainIndexBuffer);
		
		_vertexArrayObjects.push_back(vao);
	}
}

void Scene::cleanupGeometry()
{
	auto meshes = childrenOfType(ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		if (!mesh->hasFlag(s3d::Flag_Helper))
		{
			mesh->setParent(nullptr);
		}
	}
	_storage.flush();
}

void Scene::cleanUpSupportMehses()
{
	auto meshes = childrenOfType(ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		if (mesh->hasFlag(s3d::Flag_Helper))
		{
			mesh->setParent(nullptr);
		}
	}
	_storage.flush();
}
