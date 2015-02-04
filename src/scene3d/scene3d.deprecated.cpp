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

void Scene::serialize(std::ostream& stream, StorageFormat fmt, const std::string& basePath)
{
	if (stream.fail()) return;

	Element::List storages = childrenOfType(ElementType_Storage);

	serializeChunk(stream, HeaderScene);
	serializeUInt32(stream, SceneVersionLatest);

	serializeChunk(stream, HeaderData);
	serializeUInt32(stream, StorageVersionLatest);

	serializeUInt32(stream, fmt);

	serializeUInt64(stream, storages.size());
	for (Scene3dStorage::Pointer s : storages)
	{
		serializeChunk(stream, HeaderMaterials);
		serializeUInt64(stream, s->materials().size());
		if (fmt == StorageFormat_Binary)
		{
			for (auto& mi : s->materials())
			{
				serializeUInt64(stream, reinterpret_cast<size_t>(mi.ptr()));
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

				serializeUInt64(stream, reinterpret_cast<size_t>(mi.ptr()));
				serializeString(stream, mFile);

				std::ofstream mStream(mFile.c_str());
				mi->serialize(mStream, fmt);
				mStream.close();
			}
		}
		else
		{
			ET_FAIL("Invalid storage format specified.");
		}

		serializeChunk(stream, HeaderVertexArrays);
		serializeUInt64(stream, s->vertexArrays().size());
		for (auto& vi : s->vertexArrays())
		{
            serializeUInt64(stream, reinterpret_cast<uintptr_t>(vi.ptr()));
			vi->serialize(stream);
		}

		IndexArray::Pointer ia = s->indexArray();
		serializeChunk(stream, HeaderIndexArrays);
		serializeUInt32(stream, 1);
        serializeUInt64(stream, reinterpret_cast<uintptr_t>(ia.ptr()));
		ia->serialize(stream);
	}

	serializeChunk(stream, HeaderElements);
	ElementContainer::serialize(stream, SceneVersionLatest);
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
	ObjectsCache& tc, const std::string& basePath, StorageFormat fmt, StorageVersion ver, bool async)
{
	Scene3dStorage::Pointer result = Scene3dStorage::Pointer::create("storage", nullptr);

	size_t readComponents = 0;
	volatile bool reading = true;
	while (reading)
	{
		ChunkId readChunk = { };
		deserializeChunk(stream, readChunk);

		if (chunkEqualTo(readChunk, HeaderMaterials))
		{
			size_t numMaterials = deserializeUInt32(stream);
			_materialsToLoad.setValue(static_cast<AtomicCounterType>(numMaterials));
			
			if (fmt == StorageFormat_Binary)
			{
				for (size_t i = 0; i < numMaterials; ++i)
				{
					Material::Pointer m;
					ET_CONNECT_EVENT(m->loaded, Scene::onMaterialLoaded);
					m->tag = deserializeInt32(stream);
					m->deserialize(stream, rc, tc, basePath, StorageFormat_Binary, async);
					result->addMaterial(m);
				}
			}
			else if (fmt == StorageFormat_HumanReadableMaterials)
			{
				for (size_t i = 0; i < numMaterials; ++i)
				{
					Material::Pointer m;
					ET_CONNECT_EVENT(m->loaded, Scene::onMaterialLoaded);
					
					m->tag = deserializeInt32(stream);
					auto materialName = deserializeString(stream);
					m->setOrigin(application().resolveFileName(basePath + getFileName(materialName)));

					InputStream mStream(m->origin(), StreamMode_Text);
					
					if (mStream.valid())
						m->deserialize(mStream.stream(), rc, tc, basePath, fmt, async);
					else
						onMaterialLoaded(nullptr);

					result->addMaterial(m);
					tc.manage(m, ObjectLoader::Pointer());
				}
			}
			else
			{
				ET_FAIL("Invalid storage format specified");
			}
			
			++readComponents;
		}
		else if (chunkEqualTo(readChunk, HeaderVertexArrays))
		{
			uint64_t numVertexArrays = 0;
			
			if (ver < StorageVersion_1_1_0)
				numVertexArrays = deserializeUInt32(stream);
			else
				numVertexArrays = deserializeUInt64(stream);
			
			for (size_t i = 0; i < numVertexArrays; ++i)
			{
				VertexArray::Pointer va = VertexArray::Pointer::create();
				va->tag = deserializeInt32(stream);
				va->deserialize(stream);
				result->addVertexArray(va);
			}
			
			++readComponents;
		}
		else if (chunkEqualTo(readChunk, HeaderIndexArrays))
		{
			int num = deserializeInt32(stream);
			ET_ASSERT(num == 1);
			(void)(num);
			
			auto indexArray = result->indexArray();
			indexArray->tag = deserializeInt32(stream);
			indexArray->deserialize(stream);
			
			++readComponents;
		}
		else
		{
			log::error("Invalid chunk in scene storage");
			result.reset(nullptr);
			return result;
		}
		
		reading = (readComponents < 3) && stream.good();
	}
	
	if (result->vertexStorages().empty())
	{
		for (const auto& va : result->vertexArrays())
		{
			auto vs = VertexStorage::Pointer::create(va);
			vs->tag = va->tag;
			
			result->addVertexStorage(vs);
		}
	}

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
		{
			ib = rc->vertexBufferFactory().createIndexBuffer(ibName, p->indexArray(), BufferDrawType::Static);
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

VertexStorage::Pointer Scene::vertexStorageForVertexBuffer(const std::string& vbName)
{
	Element::List storages = childrenOfType(ElementType_Storage);
	for (Scene3dStorage::Pointer storage : storages)
	{
		for (const auto& va : storage->vertexArrays())
		{
			auto suggestedName = "vb-" + intToStr(va->tag);
			if (suggestedName == vbName)
			{
				for (const auto& vs : storage->vertexStorages())
				{
					if (vs->tag == va->tag)
						return vs;
				}
			}
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
	for (auto& i : _vaos)
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
	
	if (stream.fail()) 
    {
		log::error("Unable to deserialize scene from stream.");
		INVOKE_FAIL
    }

	ChunkId headerChunk = { };
	deserializeChunk(stream, headerChunk);
	if (!chunkEqualTo(headerChunk, HeaderScene))
	{
		log::error("Data not looks like proper ETM file.");
		INVOKE_FAIL
	}

	uint32_t version = deserializeUInt32(stream);
	if (version > static_cast<size_t>(SceneVersionLatest))
	{
		log::error("Unsupported version of the ETM file.");
		INVOKE_FAIL
	}

	_componentsToLoad.setValue(2);
	volatile bool readCompleted = false;
	while (!readCompleted)
	{
		ChunkId readChunk = { };
		deserializeChunk(stream, readChunk);
		if (chunkEqualTo(readChunk, HeaderData))
		{
			uint32_t storageVersion = deserializeUInt32(stream);
			
			size_t numStorages = 0;
			StorageFormat format = StorageFormat_Binary;
			
			if (storageVersion == StorageVersion_1_0_0)
			{
				numStorages = deserializeUInt32(stream);
			}
			else if (storageVersion >= StorageVersion_1_0_1)
			{
				format = static_cast<StorageFormat>(deserializeInt32(stream));
				if (storageVersion >= StorageVersion_1_1_0)
					numStorages = deserializeUInt64(stream);
				else
					numStorages = deserializeUInt32(stream);
			}
			else
			{
				log::error("Unsupported version of binary storage the ETM file.");
				INVOKE_FAIL
			}

			for (size_t i = 0; i < numStorages; ++i)
			{
				Scene3dStorage::Pointer ptr = deserializeStorage(stream, rc, tc, basePath, format,
					static_cast<StorageVersion>(storageVersion), async);
				ptr->setParent(this);
			}

		}
		else if (chunkEqualTo(readChunk, HeaderElements))
		{
			ElementContainer::deserialize(stream, this, static_cast<SceneVersion>(version));
			readCompleted = true;
		}
		else
		{
			log::error("Unknown chunk in ETM file.");
			INVOKE_FAIL
		}
	}
	_componentsToLoad.release();
	
	if (_componentsToLoad.atomicCounterValue() == 0)
		deserializationFinished.invoke(true);

	return true;
}
