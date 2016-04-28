/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/mesh.h>
#include <et/scene3d/scenematerial.h>
#include <et/scene3d/storage.h>
#include <et/scene3d/modelloader.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/vertexstorage.h>

namespace et
{
	class MaterialProvider;
	class OBJLoaderThread;
	class OBJLoader : public ModelLoader
	{
	public:
		enum Options
		{
			Option_JustLoad = 0x00,
			Option_SwapYwithZ = 0x01,
			Option_ReverseTriangles = 0x02,
			Option_CalculateTransforms = 0x04,
			Option_CalculateTangents = 0x08,
		};

	public:
		OBJLoader(const std::string& inFile, size_t options);
		~OBJLoader();

		s3d::ElementContainer::Pointer load(RenderContext*, MaterialProvider*, s3d::Storage&, ObjectsCache&);
		void loadAsync(et::RenderContext*, s3d::Storage&, ObjectsCache& cahce);

		ET_DECLARE_EVENT1(loaded, s3d::ElementContainer::Pointer)

	private:
		struct OBJMeshIndexBounds
		{
			std::string name;
			uint32_t start = 0;
			uint32_t count = 0;
			et::vec3 center;
			s3d::SceneMaterial::Pointer material;

			OBJMeshIndexBounds(const std::string& n, uint32_t s, uint32_t c, s3d::SceneMaterial::Pointer m, const vec3& aCenter) :
				name(n), start(s), count(c), center(aCenter), material(m) { }
		};

		struct OBJFace
		{
			enum
			{
				MaxVertexLinks = 16,
				MaxVertexSize = 3
			};
			using VertexLink = StaticDataStorage<size_t, MaxVertexSize>;

			StaticDataStorage<VertexLink, MaxVertexLinks> vertexLinks;
			uint32_t vertexLinksCount = 0;
			uint32_t smoothingGroupIndex = 0;
		};

		struct OBJGroup
		{
			std::string name;
			std::string material;
			Vector<OBJFace> faces;
			
			OBJGroup()
				{ faces.reserve(0xffff); }
			
			OBJGroup(const std::string& aName) :
				name(aName) { faces.reserve(0xffff); }
			
			OBJGroup(const std::string& aName, const std::string& aMat) :
				name(aName), material(aMat) { faces.reserve(0xffff); }
		};

	private:
		void loadData(bool async, ObjectsCache& cache);
		void processLoadedData();
		
		s3d::ElementContainer::Pointer generateVertexBuffers(s3d::Storage&);

		void loadMaterials(const std::string& fileName, bool async, ObjectsCache& cache);
		void threadFinished();

	private:
		friend class OBJLoaderThread;

		RenderContext* _rc = nullptr;
		MaterialProvider* _materialProvider = nullptr;
		UniquePtr<OBJLoaderThread> _thread;

		std::string inputFileName;
		std::string inputFilePath;
		std::ifstream inputFile;
		std::ifstream materialFile;

		s3d::SceneMaterial::Pointer _lastMaterial;
		s3d::SceneMaterial::Collection _materials;
		Vector<OBJMeshIndexBounds> _meshes;
		
        IndexArray::Pointer _indices;
		VertexStorage::Pointer _vertexData;

		Vector<et::vec3> _vertices;
		Vector<et::vec3> _normals;
		Vector<et::vec2> _texCoords;
		Vector<OBJGroup> _groups;

        size_t lastGroupIndex = static_cast<size_t>(-1);
		size_t _loadOptions = Option_JustLoad;
		int _lastSmoothGroup = 0;
		int _lastGroupId = 0;
	};
}
