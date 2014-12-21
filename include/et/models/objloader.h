/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <fstream>
#include <et/apiobjects/vertexbuffer.h>
#include <et/scene3d/mesh.h>
#include <et/scene3d/supportmesh.h>
#include <et/scene3d/material.h>
#include <et/scene3d/storage.h>
#include <et/rendering/rendercontext.h>

namespace et
{
	class OBJLoaderThread;
	class OBJLoader
	{
	public:
		enum Options
		{
			Option_SupportMeshes = 0x01,
			Option_SwapYwithZ = 0x02,
			Option_ReverseTriangles = 0x04,
			Option_CalculateTransforms = 0x80,
		};

	public:
		OBJLoader(RenderContext* rc, const std::string& inFile);
		~OBJLoader();

		s3d::ElementContainer::Pointer load(ObjectsCache& cahce, size_t options);
		void loadAsync(ObjectsCache& cahce);

		ET_DECLARE_EVENT1(loaded, s3d::ElementContainer::Pointer)

	private:
		struct OBJVertex
		{
			size_t numVertices = 0;
			StaticDataStorage<size_t, 3> vertices;

			size_t& operator [] (size_t i)
				{ ET_ASSERT(i < numVertices); return vertices[i]; }

			const size_t& operator [] (size_t i) const
				{ ET_ASSERT(i < numVertices); return vertices[i]; }
			
			OBJVertex() : numVertices(0)
				{ vertices.fill(0); }
		};

		struct OBJMeshIndexBounds
		{
			std::string name;
			IndexType start;
			size_t count;
			et::vec3 center;
			s3d::Material::Pointer material;

			OBJMeshIndexBounds(const std::string& n, IndexType s, size_t c, s3d::Material::Pointer m, const vec3& aCenter) :
				name(n), start(s), count(c), material(m), center(aCenter) { }
		};
		typedef std::vector<OBJMeshIndexBounds> OBJMeshIndexBoundsList;
		typedef std::vector<OBJVertex> VertexList;

		struct OBJFace
		{
			int smoothingGroupIndex;

			VertexList vertices;

			OBJFace() : 
				smoothingGroupIndex(0) { }
		};
		typedef std::vector<OBJFace> FaceList;

		struct OBJGroup
		{
			std::string name;
			std::string material;
			FaceList faces;
			
			OBJGroup()
				{ }
			
			OBJGroup(const std::string& aName) :
				name(aName) { }
			
			OBJGroup(const std::string& aName, const std::string& aMat) :
				name(aName), material(aMat) { }
		};

	private:
		void loadData(bool async, ObjectsCache& cache);
		void processLoadedData();
		s3d::ElementContainer::Pointer generateVertexBuffers();

		void loadMaterials(const std::string& fileName, bool async, ObjectsCache& cache);
		void threadFinished();

	private:
		friend class OBJLoaderThread;

		RenderContext* _rc = nullptr;
		AutoPtr<OBJLoaderThread> _thread;

		std::string inputFileName;
		std::string inputFilePath;
		std::ifstream inputFile;
		std::ifstream materialFile;

		s3d::Scene3dStorage::Pointer _storage;
		s3d::Material::Pointer _lastMaterial;
		s3d::Material::List _materials;
		OBJMeshIndexBoundsList _meshes;
        IndexArray::Pointer _indices;
		VertexArray::Pointer _vertexData;

		std::vector<et::vec3, et::SharedBlockAllocatorSTDProxy<et::vec3>> _vertices;
		std::vector<et::vec3, et::SharedBlockAllocatorSTDProxy<et::vec3>> _normals;
		std::vector<et::vec2, et::SharedBlockAllocatorSTDProxy<et::vec2>> _texCoords;
		std::vector<OBJGroup*,et::SharedBlockAllocatorSTDProxy<OBJGroup*>> _groups;

		OBJGroup* lastGroup = nullptr;
		size_t _loadOptions = 0;
		int _lastSmoothGroup = 0;
		int _lastGroupId = 0;
	};
}
