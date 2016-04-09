/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/geometry/collision.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/primitives.h>
#include <et-ext/helpers/terraindata.h>

namespace et
{
	class Terrain;
	class TerrainChunk;
	class TerrainLODLevels;

	class TerrainDelegate : public TerrainDataDelegate
	{
	public: 
		virtual int computeTerrainLod(Terrain const*, const BoundingBox&/* chunkAABB */, int/* lastLod */, bool& /* visible */)
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

		VertexBuffer::Pointer _vertexBuffer;
		IndexBuffer::Pointer _indexBuffer;
		VertexArrayObject::Pointer _vao;

		vec2i _chunkSizes;
	};

	typedef IntrusivePtr<Terrain> TerrainRef;
}
