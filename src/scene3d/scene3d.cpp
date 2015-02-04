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

void Scene::serialize(std::ostream& stream, const std::string& basePath)
{
	Element::List storages = childrenOfType(ElementType_Storage);
	for (Scene3dStorage::Pointer storage : storages)
	{
		for (auto m : storage->materials())
		{
			m->toDictionary().printContent();
		}
	}
}

void Scene::deserializeAsync(std::istream& stream, RenderContext* rc, ObjectsCache& tc, const std::string& basePath)
{
	performDeserialization(stream, rc, tc, basePath, true);
}

void Scene::deserializeAsync(const std::string& filename, RenderContext* rc, ObjectsCache& tc)
{
	InputStream file(filename, StreamMode_Binary);
	deserializeAsync(file.stream(), rc, tc, getFilePath(filename));
}

bool Scene::deserialize(std::istream& stream, RenderContext* rc, ObjectsCache& tc, const std::string& basePath)
{
	return performDeserialization(stream, rc, tc, basePath, false);
}

Scene3dStorage::Pointer Scene::deserializeStorage(std::istream& stream, RenderContext* rc,
	ObjectsCache& tc, const std::string& basePath, StorageVersion ver, bool async)
{
	Scene3dStorage::Pointer result = Scene3dStorage::Pointer::create("storage", nullptr);
	buildAPIObjects(result, rc);
	return result;
}

void Scene::buildAPIObjects(Scene3dStorage::Pointer p, RenderContext* rc)
{
	IndexBuffer ib;
	
	for (const auto& vs : p->vertexStorages())
	{
		std::string vbName = "vb-" + intToStr(static_cast<uint32_t>(vs->tag));
		std::string ibName = "ib-" + intToStr(static_cast<uint32_t>(p->indexArray()->tag));
		std::string vaoName = "vao-" + intToStr(static_cast<uint32_t>(vs->tag)) + "-" +
			intToStr(static_cast<uint32_t>(p->indexArray()->tag));
		
		VertexArrayObject vao = rc->vertexBufferFactory().createVertexArrayObject(vaoName);
		
		VertexBuffer::Pointer vb = rc->vertexBufferFactory().createVertexBuffer(vbName, vs, BufferDrawType::Static);
		
		if (!ib.valid())
			ib = rc->vertexBufferFactory().createIndexBuffer(ibName, p->indexArray(), BufferDrawType::Static);
		
		vao->setBuffers(vb, ib);
		_vertexArrayObjects.push_back(vao);
	}

	rc->renderState().resetBufferBindings();
}

void Scene::serialize(const std::string& filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out);
	serialize(file, getFilePath(filename));
}

bool Scene::deserialize(const std::string& filename, RenderContext* rc, ObjectsCache& tc)
{
	InputStream file(application().resolveFileName(filename), StreamMode_Binary);
	
	if (file.invalid())
		return false;
	
	bool success = deserialize(file.stream(), rc, tc, getFilePath(filename));

	if (!success)
		log::error("Unable to load scene from file: %s", filename.c_str());

	return success;
}

Element::Pointer Scene::createElementOfType(size_t type, Element* parent)
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
		return Scene3dStorage::Pointer::create(emptyString, parent);
	case ElementType_Camera:
		return CameraElement::Pointer::create(emptyString, parent);
	default:
		return ElementContainer::Pointer::create(emptyString, parent);
	}
}

IndexArray::Pointer Scene::primaryIndexArray()
{
	Element::List storages = childrenOfType(ElementType_Storage);
	ET_ASSERT(!storages.empty())
	return Scene3dStorage::Pointer(storages.front())->indexArray();
}

VertexStorage::Pointer Scene::vertexStorageWithName(const std::string& vsName)
{
	Element::List storages = childrenOfType(ElementType_Storage);
	for (Scene3dStorage::Pointer storage : storages)
	{
		for (const auto& vs : storage->vertexStorages())
		{
			if (vs->name() == vsName)
				return vs;
		}
	}
	return VertexStorage::Pointer();
}

Material::Pointer Scene::materialWithId(uint64_t id)
{
	Element::List storages = childrenOfType(ElementType_Storage);
	for (Scene3dStorage::Pointer storage : storages)
	{
		for (auto& data : storage->materials())
		{
			if (data->tag == id)
				return data;
		}
	}
	
	return Material::Pointer();
}

void Scene::onMaterialLoaded(Material*)
{
	ET_ASSERT(_materialsToLoad.atomicCounterValue() > 0)
	
	_materialsToLoad.release();
	
	if (_materialsToLoad.atomicCounterValue()== 0)
		allMaterialsLoaded();
}

void Scene::allMaterialsLoaded()
{
	ET_ASSERT(_componentsToLoad.atomicCounterValue() > 0)
	
	_componentsToLoad.release();
	
	if (_componentsToLoad.atomicCounterValue() == 0)
		deserializationFinished.invoke(true);
}

VertexArrayObject Scene::vaoWithIdentifiers(const std::string& vbid, const std::string& ibid)
{
	for (auto& i : _vertexArrayObjects)
	{
		if ((i->vertexBuffer()->name() == vbid) && (i->indexBuffer()->name() == ibid))
			return i;
	}

	return VertexArrayObject();
}

#define INVOKE_FAIL		{ if (async) { deserializationFinished.invoke(false); } return false; }

bool Scene::performDeserialization(std::istream& stream, RenderContext* rc, ObjectsCache& tc,
	const std::string& basePath, bool async)
{
	clearRecursively();
	deserializationFinished.invoke(true);
	return true;
}
