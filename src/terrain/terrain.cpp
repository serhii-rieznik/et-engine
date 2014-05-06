/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <fstream>
#include <et/core/tools.h>
#include <et/app/application.h>
#include <et/primitives/primitives.h>
#include <et/terrain/terrain.h>
#include <et/timers/intervaltimer.h>

using namespace et;

#define ET_TERRAIN_CHUNK_SIZE		32

class et::TerrainLODLevels
{
public:
	struct LOD
	{
		static const size_t VARIATIONS = 16;
		size_t first[VARIATIONS];
		size_t size[VARIATIONS];
	};

	LOD lods[Terrain::LodLevel_max];

	enum TriangleType
	{
		TT_LeftTop,
		TT_LeftBottom,
		TT_RightTop,
		TT_RightBottom
	};

	enum LODSide
	{
		Side_Left,
		Side_Right,
		Side_Bottom,
		Side_Top
	};

	enum LODCorner
	{
		Corner_LeftTop,
		Corner_LeftBottom,
		Corner_RightTop,
		Corner_RightBottom
	};

	union LODVariation
	{ 
		struct 
		{
			bool conjugateLeft : 1;
			bool conjugateRight : 1;
			bool conjugateTop : 1;
			bool conjugateBottom : 1;
		};

		struct 
		{
			int _definition; 
		};

		inline int definition() 
		{ 
			return static_cast<int>(conjugateLeft) | static_cast<int>(conjugateRight) << 1 | 
					static_cast<int>(conjugateTop) << 2 | static_cast<int>(conjugateBottom) << 3; 
		}

		LODVariation() : _definition(0) { }
	};

	typedef unsigned int TerrainIndexType;

	inline void pushIndex(TerrainIndexType value)
	{
		IndexType t = static_cast<IndexType>(value);
		_buf->push_back(t);
	}

	void putTriangle(TriangleType t, TerrainIndexType c_u, TerrainIndexType n_u, TerrainIndexType c_v, TerrainIndexType n_v)
	{
		switch (t) 
		{
		case TT_LeftTop:
			{
				pushIndex(c_u + c_v);
				pushIndex(c_u + n_v);
				pushIndex(n_u + c_v);
				break;
			}
		case TT_RightTop:
			{
				pushIndex(c_u + c_v);
				pushIndex(n_u + n_v);
				pushIndex(n_u + c_v);
				break;
			}
		case TT_LeftBottom:
			{
				pushIndex(c_u + c_v);
				pushIndex(c_u + n_v);
				pushIndex(n_u + n_v);
				break;
			}
		case TT_RightBottom:
			{
				pushIndex(n_u + c_v);
				pushIndex(c_u + n_v);
				pushIndex(n_u + n_v);
				break;
			}
		};
	}

	void generateSolidLOD(int level, size_t var)
	{
		TerrainIndexType scale = static_cast<TerrainIndexType>(intPower(2, level));
		vector2<TerrainIndexType> terrainDim(static_cast<TerrainIndexType>(_t->terrainData()->dimension().x - 1), 
									  static_cast<TerrainIndexType>(_t->terrainData()->dimension().y - 1));
		vector2<TerrainIndexType> dim(ET_TERRAIN_CHUNK_SIZE / scale, ET_TERRAIN_CHUNK_SIZE / scale);
		vec2i s(_x0, _z0);

		lods[level].first[var] = _buf->actualSize();
		lods[level].size[var] = dim.square() * 6;

		for (TerrainIndexType v = 0; v < dim.y; ++v)
		{
			TerrainIndexType c_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + v * scale)) * (terrainDim.x + 1);
			TerrainIndexType n_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + (v + 1) * scale)) * (terrainDim.x + 1);

			for (TerrainIndexType u = 0; u < dim.x; ++u)
			{
				TerrainIndexType c_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + u * scale));
				TerrainIndexType n_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + (u + 1) * scale));

				pushIndex(c_u + c_v);
				pushIndex(c_u + n_v);
				pushIndex(n_u + c_v);
				pushIndex(c_u + n_v);
				pushIndex(n_u + n_v);
				pushIndex(n_u + c_v);
			}
		}
	}

	void generateLODInterior(const vec2i& lod_dim, int lod_scale_hi, int lod_scale, int& TRI_COUNTER)
	{
		vector2<TerrainIndexType> terrainDim(static_cast<TerrainIndexType>(_t->terrainData()->dimension().x - 1), 
									    static_cast<TerrainIndexType>(_t->terrainData()->dimension().y - 1));

		for (TerrainIndexType v = 0; v < static_cast<TerrainIndexType>(lod_dim.y - 1); ++v)
		{
			TerrainIndexType c_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + lod_scale_hi + v * lod_scale)) * (terrainDim.x + 1);
			TerrainIndexType n_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + lod_scale_hi + (v + 1) * lod_scale)) * (terrainDim.x + 1);
			for (TerrainIndexType u = 0; u < static_cast<TerrainIndexType>(lod_dim.x - 1); ++u)
			{
				TerrainIndexType c_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + lod_scale_hi + u * lod_scale));
				TerrainIndexType n_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + lod_scale_hi + (u + 1) * lod_scale ));
				pushIndex(c_u + c_v);
				pushIndex(c_u + n_v);
				pushIndex(n_u + c_v);
				pushIndex(c_u + n_v);
				pushIndex(n_u + n_v);
				pushIndex(n_u + c_v);
				TRI_COUNTER += 2;
			}
		}
	}

	void putTransitionTriangles(LODSide side, const vec2i& lod_dim_hi, const vec2i& lod_dim, 
			TerrainIndexType lod_scale_hi, TerrainIndexType lod_scale, int& TRI_COUNTER)
	{
		vector2<TerrainIndexType> terrainDim(static_cast<TerrainIndexType>(_t->terrainData()->dimension().x - 1), 
									  static_cast<TerrainIndexType>(_t->terrainData()->dimension().y - 1));
		if ( (side == Side_Top) || (side == Side_Bottom) )
		{
			for (TerrainIndexType u = 0; u < static_cast<TerrainIndexType>(lod_dim.x - 1); ++u)
			{
				TerrainIndexType v = (side == Side_Top) ? 0 : static_cast<TerrainIndexType>(lod_dim_hi.y - 1);
				TerrainIndexType c_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + v * lod_scale_hi)) * (terrainDim.x + 1);
				TerrainIndexType n_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + (v + 1) * lod_scale_hi)) * (terrainDim.x + 1);
				TerrainIndexType c_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + lod_scale_hi + u * lod_scale));
				TerrainIndexType m_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + lod_scale_hi + u * lod_scale + lod_scale_hi));
				TerrainIndexType n_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + lod_scale_hi + (u + 1) * lod_scale));
				if (side == Side_Top)
				{
					pushIndex( m_u + c_v );
					pushIndex( c_u + n_v );
					pushIndex( n_u + n_v );
				}
				else
				{
					pushIndex( c_u + c_v );
					pushIndex( m_u + n_v );
					pushIndex( n_u + c_v );
				}
				++TRI_COUNTER;
			}
		}
		else
		{
			for (int v = 0; v < lod_dim.y - 1; ++v)
			{
				TerrainIndexType u = (side == Side_Left) ? 0 : static_cast<TerrainIndexType>((lod_dim.x - 1) * lod_scale + lod_scale_hi);
				TerrainIndexType c_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + lod_scale_hi + v * lod_scale)) * (terrainDim.x + 1);
				TerrainIndexType m_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + lod_scale_hi + v * lod_scale + lod_scale_hi)) * (terrainDim.x + 1);
				TerrainIndexType n_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + lod_scale_hi + (v + 1) * lod_scale)) * (terrainDim.x + 1);
				TerrainIndexType c_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + u));
				TerrainIndexType n_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + u + lod_scale_hi));
				if (side == Side_Left)
				{
					pushIndex( n_u + c_v );
					pushIndex( c_u + m_v );
					pushIndex( n_u + n_v );
				}
				else
				{
					pushIndex( c_u + c_v );
					pushIndex( c_u + n_v );
					pushIndex( n_u + m_v );
				}
				++TRI_COUNTER;
			}
		}
	}

	void putConjugateTriangles(LODSide side, bool large, const vec2i& lod_dim_hi, const vec2i& lod_dim, int lod_scale_hi, int lod_scale, int& TRI_COUNTER)
	{
		int currentScale = large ? lod_scale : lod_scale_hi;
		const vec2i& currentDim = large ? lod_dim : lod_dim_hi;
		vector2<TerrainIndexType> terrainDim(static_cast<TerrainIndexType>(_t->terrainData()->dimension().x - 1), 
									  static_cast<TerrainIndexType>(_t->terrainData()->dimension().y - 1));

		if ((side == Side_Top) || (side == Side_Bottom))
		{
			int v = (side == Side_Top) ? 0 : lod_dim_hi.y - 1;
			TerrainIndexType c_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + v * lod_scale_hi)) * (terrainDim.x + 1);
			TerrainIndexType n_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + (v + 1) * lod_scale_hi)) * (terrainDim.x + 1);

			for (TerrainIndexType u = 0; u < static_cast<TerrainIndexType>(currentDim.x); ++u)
			{
				bool odd = u % 2 == 1;
				TerrainIndexType c_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + u * currentScale));
				TerrainIndexType n_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + (u + 1) * currentScale));

				if (side == Side_Top)
				{
					if (large)
					{
						TerrainIndexType m_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + u * currentScale + lod_scale_hi));
						pushIndex( c_u + c_v );
						pushIndex( m_u + n_v );
						pushIndex( n_u + c_v );
					}
					else
						putTriangle( odd ? TT_LeftTop : TT_RightTop, c_u, n_u, c_v, n_v);
				}
				else
				{
					if (large)
					{
						TerrainIndexType m_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + u * currentScale + lod_scale_hi));
						pushIndex( m_u + c_v );
						pushIndex( c_u + n_v );
						pushIndex( n_u + n_v );
					}
					else
						putTriangle( odd ? TT_LeftBottom : TT_RightBottom, c_u, n_u, c_v, n_v);
				}
				++TRI_COUNTER;
			}
		} 
		else
		{
			TerrainIndexType u = (side == Side_Left) ? 0 : static_cast<TerrainIndexType>(lod_dim_hi.x - 1);
			TerrainIndexType c_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + u * lod_scale_hi));
			TerrainIndexType n_u = etMin(terrainDim.x, static_cast<TerrainIndexType>(_x0 + (u + 1) * lod_scale_hi));

			for (int h = 0; h < currentDim.y; ++h)
			{
				bool odd = h % 2 == 1;
				TerrainIndexType c_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + h * currentScale)) * (terrainDim.x + 1);
				TerrainIndexType n_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + (h + 1) * currentScale)) * (terrainDim.x + 1);

				if (side == Side_Left)
				{
					if (large)
					{
						TerrainIndexType m_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + h * currentScale + lod_scale_hi)) * (terrainDim.x + 1);
						pushIndex(c_u + c_v);
						pushIndex(c_u + n_v);
						pushIndex(n_u + m_v);
					}
					else
						putTriangle( odd ? TT_LeftTop : TT_LeftBottom, c_u, n_u, c_v, n_v );
				}
				else
				{
					if (large)
					{
						TerrainIndexType m_v = etMin(terrainDim.y, static_cast<TerrainIndexType>(_z0 + h * currentScale + lod_scale_hi)) * (terrainDim.x + 1);
						pushIndex(n_u + c_v);
						pushIndex(c_u + m_v);
						pushIndex(n_u + n_v);
					}
					else
					{
						putTriangle( odd ? TT_RightTop : TT_RightBottom, c_u, n_u, c_v, n_v );
					}
				}
			}
			++TRI_COUNTER;
		}
	}

	void generateLODLevel(int level, LODVariation var)
	{
		if (var.definition() == 0)
		{
			generateSolidLOD(level, 0);
			return;
		}

		int variation = var.definition();
		TerrainIndexType lod_scale = static_cast<TerrainIndexType>(intPower(2, level));
		TerrainIndexType lod_scale_hi = lod_scale / 2;
		vec2i lod_dim = vec2i( ET_TERRAIN_CHUNK_SIZE / lod_scale, ET_TERRAIN_CHUNK_SIZE / lod_scale );
		vec2i lod_dim_hi = 2 * lod_dim;

		lods[level].first[variation] = _buf->actualSize();

		int TRI_COUNTER = 0;

		putConjugateTriangles(Side_Left, !var.conjugateLeft, lod_dim_hi, lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);
		putConjugateTriangles(Side_Top, !var.conjugateTop, lod_dim_hi, lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);
		putConjugateTriangles(Side_Right, !var.conjugateRight, lod_dim_hi, lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);
		putConjugateTriangles(Side_Bottom, !var.conjugateBottom, lod_dim_hi, lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);

		putTransitionTriangles(Side_Left, lod_dim_hi, lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);
		putTransitionTriangles(Side_Top, lod_dim_hi, lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);
		putTransitionTriangles(Side_Bottom, lod_dim_hi, lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);
		putTransitionTriangles(Side_Right, lod_dim_hi, lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);

		generateLODInterior(lod_dim, lod_scale_hi, lod_scale, TRI_COUNTER);

		lods[level].size[variation] = _buf->actualSize() - lods[level].first[variation];
	}

	TerrainLODLevels(IndexArray::Pointer buf, Terrain* t) : _buf(buf), _t(t), _x0(0), _z0(0)
	{
		int maxLodIndices = ET_TERRAIN_CHUNK_SIZE * ET_TERRAIN_CHUNK_SIZE * 6;
		int approxIndices = maxLodIndices;

		for (int i = 1; i < Terrain::LodLevel_max; ++i)
			approxIndices += 16 * maxLodIndices / intPower(2, i);

		_buf->resizeToFit(approxIndices);

		generateSolidLOD(0, 0);

		for (int level = 1; level < Terrain::LodLevel_max; ++level)
		{
			for (int var = 0; var < TerrainLODLevels::LOD::VARIATIONS; ++var)
			{
				TerrainLODLevels::LODVariation variation;
				variation._definition = var;
				generateLODLevel(level, variation);
			}
		}

		_buf->compact();
	}

private:
	TerrainLODLevels& operator = (const TerrainLODLevels&)
		{ return *this; }

private:
	IndexArray::Pointer _buf;
	Terrain* _t;

	TerrainIndexType _x0;
	TerrainIndexType _z0;
	TerrainIndexType _lod0EndIndex;
};

class et::TerrainChunk
{
public:
	TerrainChunk(int index, Terrain* t, int x0, int z0) : selectedLod(0), selectedVariation(0), 
		visible(true), _x0(x0), _z0(z0), _index(index), _t(t)
	{
		VertexDataChunk pos = t->_tdata->vertexData()->chunk(Usage_Position);
		RawDataAcessor<vec3> posData = pos.accessData<vec3>(0);

		initialOffset = x0 + z0 * _t->terrainData()->dimension().x;
		vec3 _minVect = posData[initialOffset];
		vec3 _maxVect = _minVect;

		for (int z = z0, ze = z0 + ET_TERRAIN_CHUNK_SIZE; z < ze; ++z)
		{
			for (int x = x0, xe = x0 + ET_TERRAIN_CHUNK_SIZE; x < xe; ++x)
			{
				int e = x + z * _t->terrainData()->dimension().x;
				_minVect = minv(posData[e], _minVect);
				_maxVect = maxv(posData[e], _maxVect);
			}
		}

		vec3 center = (_minVect + _maxVect) / 2.0f;
		vec3 dimension = _maxVect - _minVect;
		_aabb = AABB(center, dimension);
	}

public:
	int selectedLod;
	int selectedVariation;
	int initialOffset;
	bool visible;

	const int index() const
		{ return _index; }

	const AABB& aabb() const
		{ return _aabb; }

private:
	int _x0;
	int _z0;
	int _index;
	Terrain* _t;
	AABB _aabb;
};

Terrain::Terrain(RenderContext* rc, TerrainDelegate* tDelegate) : 
	_rc(rc), _tdata(0), _delegate(tDelegate), _lods(0)
{

}

Terrain::~Terrain()
{ 
	releaseData();
}

void Terrain::releaseData()
{
	for (ChunkIterator i = _chunks.begin(), e = _chunks.end(); i != e; ++i)
		delete *i;

	_chunks.clear();

	delete _lods;
	_lods = 0;
}

void Terrain::render(RenderContext* rc)
{
	rc->renderState().bindVertexArray(_vao);
	for (ChunkIterator i = _chunks.begin(), e = _chunks.end(); i != e; ++i)
	{
		TerrainChunk* chunk = *i;
		TerrainLODLevels::LOD& l = _lods->lods[chunk->selectedLod];
		rc->renderer()->drawElementsBaseIndex(_vao, chunk->initialOffset, 
			l.first[chunk->selectedVariation], l.size[chunk->selectedVariation]);
	}
}

void Terrain::loadFromData(const TerrainDataRef& data)
{
	_tdata = data;
	generateBuffer();
}

void Terrain::loadFromStream(std::istream& stream, const vec2i& dimension, TerrainData::Format format)
{
	loadFromData(TerrainDataRef(new TerrainData(_delegate, stream, dimension, format)));
}

void Terrain::loadFromRAWFile(const std::string& fileName, const vec2i& dimension, TerrainData::Format format)
{
	std::ifstream rawFile(application().resolveFileName(fileName), std::ios_base::binary);
	
	if (rawFile.good())
		loadFromStream(rawFile, dimension, format);
}

void Terrain::generateBuffer()
{
	releaseData();
	
	IndexArray::Pointer indices(new IndexArray(IndexArrayFormat_16bit, 0, PrimitiveType_Triangles));
	_lods = new TerrainLODLevels(indices, this);
	generateChunks();
	indices->compact();

	_vao = _rc->vertexBufferFactory().createVertexArrayObject("terrain-vao");
	_vertexBuffer = _rc->vertexBufferFactory().createVertexBuffer("terrain-vb", _tdata->vertexData(), BufferDrawType_Static);
	_indexBuffer = _rc->vertexBufferFactory().createIndexBuffer("terrain-ib", indices, BufferDrawType_Static);
	_vao->setBuffers(_vertexBuffer, _indexBuffer);
}

void Terrain::generateChunks()
{ 
	std::cout << "Generating terrain chunks... ";
	IntervalTimer timer(true);

	int totalChunks = 0;
	_chunkSizes = _tdata->dimension() / ET_TERRAIN_CHUNK_SIZE;
	for (int z = 0; z < _chunkSizes.y; ++z)
	{
		for (int x = 0; x < _chunkSizes.x; ++x, ++totalChunks)
		{
			TerrainChunk* c = new TerrainChunk(totalChunks, this, x * ET_TERRAIN_CHUNK_SIZE, z * ET_TERRAIN_CHUNK_SIZE);
			_chunks.push_back(c);
		}
	}

	std::cout << "done (" << timer.lap() << " sec) " << std::endl;
}

void Terrain::recomputeLodLevels()
{
	if (!_delegate) return;

	for (ChunkIterator i = _chunks.begin(), e = _chunks.end(); i != e; ++i)
	{
		bool visible = true;
		(*i)->selectedLod = clamp<int>(_delegate->computeTerrainLod(this, (*i)->aabb(), (*i)->selectedLod, visible), 0, LodLevel4);
		(*i)->visible = visible;
	}
	validateLODLevels();
}

void Terrain::validateLODLevels()
{
	for (ChunkIterator i = _chunks.begin(), e = _chunks.end(); i != e; ++i)
	{
		TerrainChunk* chunk = *i;

		if (chunk->selectedLod == 0)
		{
			chunk->selectedVariation = 0;
		}
		else
		{
			int chunkRow = chunk->index() / _chunkSizes.x;
			int chunkCol = chunk->index() % _chunkSizes.x;

			int leftCol = etMax(0, chunkCol - 1);
			int rightCol = etMin(_chunkSizes.x - 1, chunkCol + 1);
			int topRow = etMax(0, chunkRow - 1);
			int bottomRow = etMin(_chunkSizes.y - 1, chunkRow + 1);

			int leftChunk = leftCol + chunkRow * _chunkSizes.x;
			int rightChunk = rightCol + chunkRow * _chunkSizes.x;
			int topChunk = chunkCol + topRow * _chunkSizes.x;
			int bottomChunk = chunkCol + bottomRow * _chunkSizes.x;

			int ownLOD = chunk->selectedLod;

			TerrainLODLevels::LODVariation variation;
			variation.conjugateLeft = ownLOD > _chunks.at(leftChunk)->selectedLod;
			variation.conjugateRight = ownLOD > _chunks.at(rightChunk)->selectedLod;
			variation.conjugateTop = ownLOD > _chunks.at(topChunk)->selectedLod;
			variation.conjugateBottom = ownLOD > _chunks.at(bottomChunk)->selectedLod;

			chunk->selectedVariation = variation.definition();
		}
	}
}