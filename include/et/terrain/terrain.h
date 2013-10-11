/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/collision/collision.h>
#include <et/primitives/primitives.h>
#include <et/rendering/rendercontext.h>
#include <et/terrain/terraindata.h>

namespace et
{
	class Terrain;
	class TerrainChunk;
	class TerrainLODLevels;

	class TerrainDelegate : public TerrainDataDelegate
	{
	public: 
		virtual int computeTerrainLod(Terrain const*, const AABB&/* chunkAABB */, int/* lastLod */, bool& /* visible */) 
			{ return 0; };
	};

	class Terrain : public Shared
	{
	public:
		enum
		{
			LodLevel0 = 0, 
			LodLevel1, 
			LodLevel2, 
			LodLevel3, 
			LodLevel4, 
			LodLevel_max
		};

	public:
		Terrain(RenderContext* rc, TerrainDelegate* tDelegate);
		~Terrain();

		void loadFromData(const TerrainDataRef& data);
		void loadFromRAWFile(const std::string& fileName, const vec2i& dimension, TerrainData::Format format);
		void loadFromStream(std::istream& stream, const vec2i& dimension, TerrainData::Format format);

		void recomputeLodLevels();
		void render(RenderContext* rc);

		inline const TerrainDataRef terrainData() const
			{ return _tdata; }

	private:
		typedef std::vector<TerrainChunk*> TerrainChunkList;
		typedef TerrainChunkList::iterator ChunkIterator;

		void generateBuffer();
		void generateChunks();
		void validateLODLevels();

		void releaseData();

	private:
		friend class TerrainChunk;

		RenderContext* _rc;
		TerrainDataRef _tdata;
		TerrainDelegate* _delegate;
		TerrainLODLevels* _lods;
		TerrainChunkList _chunks;

		VertexBuffer _vertexBuffer;
		IndexBuffer _indexBuffer;
		VertexArrayObject _vao;

		vec2i _chunkSizes;
	};

	typedef IntrusivePtr<Terrain> TerrainRef;
}