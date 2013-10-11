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
#include <et/scene3d/material.h>
#include <et/rendering/rendercontext.h>

namespace et
{

	class OBJLoader;
	class OBJLoaderThread : public Thread
	{
	public:
		ThreadResult main();

	private:
		OBJLoaderThread(OBJLoader*, ObjectsCache&);

	private:
		friend class OBJLoader;
		OBJLoader* _owner;
		ObjectsCache& _cache;
	};
	
	class OBJLoader
	{
	public:
		struct OBJVertex
		{
			int v[3];
			inline int& operator [] (int i) { return v[i]; }
		};

		typedef std::vector<vec3> vec3List;
		typedef std::vector<vec2> vec2List;
		typedef std::vector<OBJVertex> VertexList;

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

		struct OBJFace
		{
			int smoothingGroupIndex;
			VertexList vertices;
		};
		typedef std::vector<OBJFace> FaceList;

		struct OBJGroup
		{
			std::string name;
			std::string material;
			FaceList faces;
		};
		typedef std::vector<OBJGroup*> GroupList;

	public:
		OBJLoader(RenderContext* rc, const std::string& inFile);
		~OBJLoader();

		s3d::ElementContainer::Pointer load(ObjectsCache& cahce);
		void loadAsync(ObjectsCache& cahce);

		ET_DECLARE_EVENT1(loaded, s3d::ElementContainer::Pointer)

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

		int lastSmoothGroup;
		int lastGroupId_;
		bool canConvert;
	};

	namespace obj
	{

		inline std::istream& operator >> (std::istream& stream, OBJLoader::OBJVertex& value) 
		{
			value[0] = value[1] = value[2] = -1;

			stream >> value[0];
			char delim = static_cast<char>(stream.peek());
			if (stream.peek() == '/')
			{
				stream >> delim;
				if (stream.peek() != '/')
					stream >> value[1];
			}
			delim = static_cast<char>(stream.peek());
			{
				stream >> delim;
				if (stream.peek() != '/')
					stream >> value[2];
			}
			for (int i = 0; i < 3; ++i)
				value[i] = value[i] - 1; 

			return stream;
		}

		inline std::istream& operator >> (std::istream& stream, vec2& value) 
		{
			stream >> value.x >> value.y;
			return stream;
		}

		inline std::istream& operator >> (std::istream& stream, vec3& value) 
		{
			stream >> value.x >> value.y >> value.z;
			return stream;
		}

		inline std::istream& operator >> (std::istream& stream, vec4& value) 
		{
			stream >> value.x >> value.y >> value.z;

			if (stream.peek() == ' ')
				stream >> value.w;
			else
				value.w = 1.0;

			return stream;
		}
	}
}
