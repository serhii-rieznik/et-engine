/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <fstream>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/scene3d.h>
#include <et/scene3d/serialization.h>

using namespace et;
using namespace s3d;

Scene::Scene(const std::string& name) :
	ElementContainer(name, nullptr), _externalFactory(nullptr)
{
}

void Scene::serialize(std::ostream& stream, StorageFormat fmt, const std::string& basePath)
{
	if (stream.fail()) return;

	Element::List storages = childrenOfType(ElementType_Storage);

	serializeChunk(stream, HeaderScene);
	serializeInt(stream, SceneVersionLatest);

	serializeChunk(stream, HeaderData);
	serializeInt(stream, StorageVersionLatest);

	serializeInt(stream, fmt);

	serializeInt(stream, storages.size());
	for (Scene3dStorage::Pointer s : storages)
	{
		serializeChunk(stream, HeaderMaterials);
		serializeInt(stream, s->materials().size());
		if (fmt == StorageFormat_Binary)
		{
			for (auto& mi : s->materials())
			{
                size_t miPtr = reinterpret_cast<size_t>(mi.ptr());
				serializeInt(stream, static_cast<int>(miPtr & 0xffffffff));
				mi->serialize(stream, fmt);
			}
		}
		else if (fmt == StorageFormat_HumanReadableMaterials)
		{
			std::map<std::string, int> materialsMap;
			for (auto& mi : s->materials())
			{
				std::string mFile;
				std::string matName = mi->name();
				
				if (materialsMap.find(matName) == materialsMap.end())
				{
					mFile = basePath + matName + ".xml";
					materialsMap[matName] = 1;
				}
				else
				{
					mFile = basePath + matName + intToStr(materialsMap[matName]) + ".xml";
					materialsMap[matName] = materialsMap[matName] + 1;
				}

                size_t miPtr = reinterpret_cast<size_t>(mi.ptr()) & 0xffffffff;
				serializeInt(stream, static_cast<int>(miPtr));
				serializeString(stream, mFile);

				std::ofstream mStream(mFile.c_str());
				mi->serialize(mStream, fmt);
				mStream.close();
			}
		}
		else
		{
			assert("Invalid storage format specified." && 0);
		}

		serializeChunk(stream, HeaderVertexArrays);
		serializeInt(stream, s->vertexArrays().size());
		for (auto& vi : s->vertexArrays())
		{
            serializeInt(stream, reinterpret_cast<size_t>(vi.ptr()) & 0xffffffff);
			vi->serialize(stream);
		}

		IndexArray::Pointer ia = s->indexArray();
		serializeChunk(stream, HeaderIndexArrays);
		serializeInt(stream, 1);
        serializeInt(stream, reinterpret_cast<size_t>(ia.ptr()) & 0xffffffff);
		ia->serialize(stream);
	}

	serializeChunk(stream, HeaderElements);
	ElementContainer::serialize(stream, SceneVersionLatest);
}

void Scene::deserializeAsync(std::istream& stream, RenderContext* rc, ObjectsCache& tc,
				ElementFactory* factory, const std::string& basePath)
{
	performDeserialization(stream, rc, tc, factory, basePath, true);
}

void Scene::deserializeAsync(const std::string& filename, RenderContext* rc, ObjectsCache& tc, 
	ElementFactory* factory)
{
	InputStream file(filename, StreamMode_Binary);
	deserializeAsync(file.stream(), rc, tc, factory, getFilePath(filename));
}

bool Scene::deserialize(std::istream& stream, RenderContext* rc, ObjectsCache& tc,
	ElementFactory* factory, const std::string& basePath)
{
	return performDeserialization(stream, rc, tc, factory, basePath, false);
}

Scene3dStorage::Pointer Scene::deserializeStorage(std::istream& stream, RenderContext* rc,
	ObjectsCache& tc, const std::string& basePath, StorageFormat fmt, bool async)
{
	Scene3dStorage::Pointer result(new Scene3dStorage("storage", 0));

	volatile bool materialsRead = false;
	volatile bool vertexArraysRead = false;
	volatile bool indexArrayRead = false;

	while (!(materialsRead && vertexArraysRead && indexArrayRead))
	{
		ChunkId readChunk = { };
		deserializeChunk(stream, readChunk);

		if (chunkEqualTo(readChunk, HeaderMaterials))
		{
			size_t numMaterials = deserializeUInt(stream);

			if (fmt == StorageFormat_Binary)
			{
				for (size_t i = 0; i < numMaterials; ++i)
				{
					Material m;
					m->tag = deserializeInt(stream);
					m->deserialize(stream, rc, tc, basePath, StorageFormat_Binary, async);
					result->addMaterial(m);
				}
			}
			else if (fmt == StorageFormat_HumanReadableMaterials)
			{
				for (size_t i = 0; i < numMaterials; ++i)
				{
					Material m;
					m->tag = deserializeInt(stream);
					m->setOrigin(basePath + getFileName(deserializeString(stream)));

					InputStream mStream(m->origin(), StreamMode_Text);
					
					if (mStream.valid())
						m->deserialize(mStream.stream(), rc, tc, basePath, fmt, async);

					result->addMaterial(m);
					tc.manage(m, ObjectLoader::Pointer());
				}
			}
			else
			{
				assert("Invalid storage format specified" && false);
			}
			materialsRead = true;
		}
		else if (chunkEqualTo(readChunk, HeaderVertexArrays))
		{
			size_t numVertexArrays = deserializeUInt(stream);
			for (size_t i = 0; i < numVertexArrays; ++i)
			{
				VertexArray::Pointer va(new VertexArray());
				va->tag = deserializeInt(stream);
				va->deserialize(stream);
				result->addVertexArray(va);
			}
			vertexArraysRead = true;
		}
		else if (chunkEqualTo(readChunk, HeaderIndexArrays))
		{
			int num = deserializeInt(stream);
			assert(num == 1);
			(void)(num);
			result->indexArray()->tag = deserializeInt(stream);
			result->indexArray()->deserialize(stream);
			indexArrayRead = true;
		}

		if (stream.eof()) break;
	}

	buildAPIObjects(result, rc);
	return result;
}

void Scene::buildAPIObjects(Scene3dStorage::Pointer p, RenderContext* rc)
{
	IndexBuffer ib;
	VertexArrayList& vertexArrays = p->vertexArrays();
	for (auto& i : vertexArrays)
	{
		std::string vbName = "vb-" + intToStr(static_cast<size_t>(i->tag));
		std::string ibName = "ib-" + intToStr(static_cast<size_t>(p->indexArray()->tag));
		
		std::string vaoName = "vao-" + intToStr(static_cast<size_t>(i->tag)) +
			"-" + intToStr(static_cast<size_t>(p->indexArray()->tag));
		
		VertexArrayObject vao = rc->vertexBufferFactory().createVertexArrayObject(vaoName);
		VertexBuffer vb = rc->vertexBufferFactory().createVertexBuffer(vbName, i, BufferDrawType_Static);
		if (!ib.valid())
		{
			ib = rc->vertexBufferFactory().createIndexBuffer(ibName, p->indexArray(), BufferDrawType_Static);
			_indexBuffers.push_back(ib);
		}
		vao->setBuffers(vb, ib);

		_vaos.push_back(vao);
		_vertexBuffers.push_back(vb);
	}

	rc->renderState().resetBufferBindings();
}

void Scene::serialize(const std::string& filename, s3d::StorageFormat fmt)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out);
	serialize(file, fmt, getFilePath(filename));
}

bool Scene::deserialize(const std::string& filename, RenderContext* rc, ObjectsCache& tc,
	ElementFactory* factory)
{
	InputStream file(filename, StreamMode_Binary);
	bool success = deserialize(file.stream(), rc, tc, factory, getFilePath(filename));

	if (!success)
		log::error("Unable to load scene from file: %s", filename.c_str());

	return success;
}

Element::Pointer Scene::createElementOfType(size_t type, Element* parent)
{
	switch (type)
	{
	case ElementType_Container:
		return ElementContainer::Pointer::create(std::string(), parent);

	case ElementType_Mesh:
		return Mesh::Pointer::create(std::string(), parent);

	case ElementType_SupportMesh:
		return SupportMesh::Pointer::create(std::string(), parent);

	case ElementType_Storage:
		return Scene3dStorage::Pointer::create(std::string(), parent);

	case ElementType_Camera:
		return CameraElement::Pointer::create(std::string(), parent);

	default:
		{
			if (_externalFactory)
				return _externalFactory->createElementOfType(type, parent);
			else
				return ElementContainer::Pointer::create(std::string(), parent);
		}
	}
}

Material Scene::materialWithId(int id)
{
	Element::List storages = childrenOfType(ElementType_Storage);
	for (auto si = storages.begin(), se = storages.end(); si != se; ++si)
	{
		Scene3dStorage* storage = static_cast<Scene3dStorage*>(si->ptr());
		for (auto& data : storage->materials())
		{
			if (data->tag == id)
				return data;
		}
	}
	
	return Material();
}

VertexArrayObject Scene::vaoWithIdentifiers(const std::string& vbid, const std::string& ibid)
{
	for (auto& i : _vaos)
	{
		if ((i->vertexBuffer()->name() == vbid) && (i->indexBuffer()->name() == ibid))
			return i;
	}

	return VertexArrayObject();
}

#define INVOKE_FAIL		{ if (async) { deserializationFinished.invoke(false); } return false; }
#define INVOKE_SUCCESS	{ return true; }

bool Scene::performDeserialization(std::istream& stream, RenderContext* rc, ObjectsCache& tc,
		ElementFactory* factory, const std::string& basePath, bool async)
{
	if (stream.fail()) 
    {
		log::error("Unable to deserialize scene from stream.");
		INVOKE_FAIL
    }

	ChunkId readChunk = { };
	deserializeChunk(stream, readChunk);
	if (!chunkEqualTo(readChunk, HeaderScene)) 
	{
		log::error("Data not looks like proper ETM file.");
		INVOKE_FAIL
	}

	_externalFactory = factory;

	size_t version = deserializeUInt(stream);
	if (version > static_cast<size_t>(SceneVersionLatest))
	{
		log::error("Unsupported version of the ETM file.");
		INVOKE_FAIL
	}

	volatile bool readCompleted = false;
	while (!readCompleted)
	{
		deserializeChunk(stream, readChunk);
		if (chunkEqualTo(readChunk, HeaderData))
		{
			size_t storageVersion = deserializeUInt(stream);
			
			size_t numStorages = 0;
			StorageFormat format = StorageFormat_Binary;
			
			if (storageVersion == StorageVersion_1_0_0)
			{
				numStorages = deserializeUInt(stream);
			}
			else if (storageVersion >= StorageVersion_1_0_1)
			{
				format = static_cast<StorageFormat>(deserializeInt(stream));
				numStorages = deserializeUInt(stream);
			}
			else
			{
				log::error("Unsupported version of binary storage the ETM file.");
				INVOKE_FAIL
			}

			for (size_t i = 0; i < numStorages; ++i)
			{
				Scene3dStorage::Pointer ptr = deserializeStorage(stream, rc, tc, basePath, format, async);
				ptr->setParent(this);
			}

		}
		else if (chunkEqualTo(readChunk, HeaderElements))
		{
			ElementContainer::deserialize(stream, this, static_cast<SceneVersion>(version));
			readCompleted = true;
		}
	}

	_externalFactory = nullptr;
	INVOKE_SUCCESS
}
