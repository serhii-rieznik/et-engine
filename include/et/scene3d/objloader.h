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
#include <et/rendering/rendercontext.h>
#include <et/rendering/base/vertexstorage.h>

namespace et
{
	class MaterialProvider;
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

		s3d::ElementContainer::Pointer load(RenderContext*, s3d::Storage&, ObjectsCache&) override;

		ET_DECLARE_EVENT1(loaded, s3d::ElementContainer::Pointer)

	private:
		struct OBJMeshIndexBounds
		{
			std::string name;
			uint32_t start = 0;
			uint32_t count = 0;
			et::vec3 center;
			MaterialInstance::Pointer material;

			OBJMeshIndexBounds(const std::string& n, uint32_t s, uint32_t c, MaterialInstance::Pointer m, const vec3& aCenter) :
				name(n), start(s), count(c), center(aCenter), material(m) { }
		};

		struct OBJFace
		{
            enum : size_t
			{
				MaxVertexLinks = 16,
				MaxVertexSize = 4
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
		void loadData(ObjectsCache& cache);
		void processLoadedData();
		
		s3d::ElementContainer::Pointer generateVertexBuffers(s3d::Storage&);

		void loadMaterials(const std::string& fileName, ObjectsCache& cache);
		void threadFinished();

	private:
		friend class OBJLoaderThread;

		RenderContext* _rc = nullptr;

		std::string inputFileName;
		std::string inputFilePath;
		std::ifstream inputFile;
		std::ifstream materialFile;

		MaterialInstance::Pointer _lastMaterial;
		MaterialInstance::Collection _materials;
		Vector<OBJMeshIndexBounds> _meshes;
		
        IndexArray::Pointer _indices;
		VertexStorage::Pointer _vertexData;

		Vector<et::vec3> _vertices;
		Vector<et::vec3> _normals;
		Vector<et::vec2> _texCoords;
		Vector<OBJGroup> _groups;

		size_t _loadOptions = Option_JustLoad;
		int _lastSmoothGroup = 0;
		int _lastGroupId = 0;
	};
}
