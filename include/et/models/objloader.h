/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <et/apiobjects/vertexbuffer.h>
#include <et/scene3d/mesh.h>
#include <et/scene3d/supportmesh.h>
#include <et/scene3d/material.h>
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
			size_t numVertices;

			int vertices[4];

			int& operator [] (size_t i) 
				{ assert(i < numVertices); return vertices[i]; }

			OBJVertex() : numVertices(0)
				{ etFillMemory(vertices, 0, sizeof(vertices)); }
		};

		struct OBJMeshIndexBounds
		{
			std::string name;
			IndexType start;
			size_t count;
			s3d::Material material;

			OBJMeshIndexBounds(const std::string& n, IndexType s, size_t c, s3d::Material m) :
				name(n), start(s), count(c), material(m) { }
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
		};

		typedef std::vector<OBJGroup*> GroupList;
		typedef std::vector<vec3> vec3List;
		typedef std::vector<vec2> vec2List;

	private:
		void loadData(bool async, ObjectsCache& cache);
		void processLoadedData();
		s3d::ElementContainer::Pointer generateVertexBuffers();

		void loadMaterials(const std::string& fileName, bool async, ObjectsCache& cache);
		void threadFinished();

	private:
		friend class OBJLoaderThread;

		RenderContext* _rc;
		AutoPtr<OBJLoaderThread> _thread;

		std::string inputFileName;
		std::string inputFilePath;
		std::ifstream inputFile;
		std::ifstream materialFile;

		OBJGroup* lastGroup;
		s3d::Material lastMaterial;
		s3d::Material::List materials;
		OBJMeshIndexBoundsList _meshes;
        IndexArray::Pointer _indices;
		VertexArray::Pointer _vertexData;

		vec3List vertices;
		vec3List normals;
		vec2List texCoords;
		GroupList groups;

		size_t _loadOptions;
		int lastSmoothGroup;
		int lastGroupId_;
		bool canConvert;
	};
}
