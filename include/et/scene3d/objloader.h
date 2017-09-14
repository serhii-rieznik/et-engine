/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/mesh.h>
#include <et/scene3d/storage.h>
#include <et/scene3d/modelloader.h>
#include <et/rendering/interface/renderer.h>
#include <et/rendering/base/vertexstorage.h>

namespace et
{
class MaterialProvider;
class OBJLoader : public ModelLoader
{
public:
	enum Options
	{
		Option_JustLoad = 0,
		Option_SwapYwithZ = 1 << 0,
		Option_CalculateTransforms = 1 << 1,
		Option_CalculateTangents = 1 << 2,
	};

public:
	OBJLoader(const std::string& inFile, uint32_t options);
	~OBJLoader();

	s3d::ElementContainer::Pointer load(RenderInterface::Pointer, s3d::Storage&, ObjectsCache&) override;

	ET_DECLARE_EVENT1(loaded, s3d::ElementContainer::Pointer);

private:
	struct OBJMeshIndexBounds
	{
		std::string name;
		uint32_t start = 0;
		uint32_t count = 0;
		et::vec3 center;
		MaterialInstance::Pointer material;

		OBJMeshIndexBounds(const std::string& n, uint32_t s, uint32_t c, MaterialInstance::Pointer m, const vec3& aCenter) :
			name(n), start(s), count(c), center(aCenter), material(m)
		{
		}
	};

	struct OBJFace
	{
		enum : uint32_t
		{
			MaxVertexLinks = 8,
			MaxVertexSize = 4
		};
		using VertexLink = StaticDataStorage<uint32_t, MaxVertexSize>;
		using VertexLinks = StaticDataStorage<VertexLink, MaxVertexLinks>;
		
		VertexLinks vertexLinks;
		uint32_t vertexLinksCount = 0;
		uint32_t smoothingGroupIndex = 0;
	};

	struct OBJGroup
	{
		enum : uint32_t
		{
			MaxGroupName = 128,
			MaxMaterialName = 128,
		};
		char name[MaxGroupName] = { };
		char material[MaxMaterialName] = {};
		std::vector<OBJFace> faces;

		OBJGroup(uint64_t sizeEstimate = 256)
		{
			// log::info("OBJGroup()");
			faces.reserve(sizeEstimate);
		}

		OBJGroup(const char* aName, uint64_t sizeEstimate = 256) :
			OBJGroup(sizeEstimate)
		{
			strncpy(name, aName, std::min(strlen(aName), static_cast<size_t>(MaxGroupName)));
		}

		OBJGroup(const char* aName, const char* aMat, uint64_t sizeEstimate = 256) :
			OBJGroup(sizeEstimate)
		{
			strncpy(name, aName, std::min(strlen(aName), static_cast<size_t>(MaxGroupName)));
			strncpy(material, aMat, std::min(strlen(aMat), static_cast<size_t>(MaxMaterialName)));
		}
		OBJGroup(const std::string& aName, uint64_t sizeEstimate = 256) :
			OBJGroup(sizeEstimate)
		{
			strncpy(name, aName.data(), std::min(aName.size(), static_cast<size_t>(MaxGroupName)));
		}

		OBJGroup(const std::string& aName, const std::string& aMat, uint32_t sizeEstimate = 256) :
			OBJGroup(sizeEstimate)
		{
			strncpy(name, aName.data(), std::min(aName.size(), static_cast<size_t>(MaxGroupName)));
			strncpy(material, aMat.data(), std::min(aMat.size(), static_cast<size_t>(MaxMaterialName)));
		}
	};

private:
	void loadData(ObjectsCache& cache);
	void load(ObjectsCache& cache);
	
	bool loadCached(const std::string& fileName);
	void saveCache(const std::string& fileName);
	
	void processLoadedData();

	s3d::ElementContainer::Pointer generateVertexBuffers(s3d::Storage&);

	void loadMaterials(const std::string& fileName, ObjectsCache& cache);
	void threadFinished();

private:
	RenderInterface::Pointer _renderer;

	std::string inputFileName;
	std::string cacheFileName;
	std::string inputFilePath;
	std::ifstream inputFile;
	std::ifstream materialFile;

	MaterialInstance::Pointer _lastMaterial;
	MaterialInstance::Collection _materials;
	Vector<OBJMeshIndexBounds> _meshes;

	IndexArray::Pointer _indices;
	VertexStorage::Pointer _vertexData;

	std::vector<et::vec3> _vertices;
	std::vector<et::vec3> _normals;
	std::vector<et::vec2> _texCoords;
	std::vector<OBJGroup> _groups;
	Set<std::string> _loadedMaterials;
	char _lastUsedMaterial[256]{ };

	uint32_t _loadOptions = Option_JustLoad;
	uint64_t _sizeEstimate = 1024;
	int _lastSmoothGroup = 0;
	int _lastGroupId = 0;
};
}
