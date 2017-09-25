/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/geometry.h>
#include <et/rendering/base/material.h>
#include <et/rendering/base/vertexstream.h>
#include <et/rendering/base/vertexstorage.h>
#include <et/rendering/base/indexarray.h>

namespace et
{
class RenderBatch : public et::Shared
{
public:
	ET_DECLARE_POINTER(RenderBatch);

public:
	RenderBatch() = default;

	RenderBatch(const MaterialInstance::Pointer&, const VertexStream::Pointer&, uint32_t firstIndex, uint32_t indexCount);

	void construct(const MaterialInstance::Pointer&, const VertexStream::Pointer&, uint32_t firstIndex, uint32_t indexCount);
	void clear();

	MaterialInstance::Pointer& material()
		{ return _material; }
	const MaterialInstance::Pointer& material() const
		{ return _material; }
	void setMaterial(MaterialInstance::Pointer);

	VertexStream::Pointer& vertexStream()
		{ return _vertexStream; }
	const VertexStream::Pointer& vertexStream() const
		{ return _vertexStream; }

	uint32_t firstIndex() const
		{ return _firstIndex; }
	uint32_t numIndexes() const
		{ return _numIndexes; }

	VertexStorage::Pointer& vertexStorage()
		{ return _vertexStorage; }
	const VertexStorage::Pointer& vertexStorage() const
		{ return _vertexStorage; }
	void setVertexStorage(VertexStorage::Pointer vs)
		{ _vertexStorage = vs; }

	IndexArray::Pointer& indexArray()
		{ return _indexArray; }
	const IndexArray::Pointer& indexArray() const
		{ return _indexArray; }
	void setIndexArray(IndexArray::Pointer ia)
		{ _indexArray = ia; }

	void calculateBoundingBox();

	const BoundingBox& boundingBox() const
		{ return _boundingBox; }

	RayIntersection intersectsLocalSpaceRay(const ray3d&) const;
	Dictionary serialize() const;

	RenderBatch* duplicate() const;

private:
	MaterialInstance::Pointer _material;
	VertexStream::Pointer _vertexStream;
	VertexStorage::Pointer _vertexStorage;
	IndexArray::Pointer _indexArray;
	BoundingBox _boundingBox;
	uint32_t _firstIndex = 0;
	uint32_t _numIndexes = 0;
};

class RenderBatchPool
{
public:
	RenderBatchPool() {
		_pool.reserve(64);
		_active.reserve(64);
	}

	void clear() {
		_pool.clear();
		_active.clear();
	}
		
	RenderBatch::Pointer allocate() {
		return acquire();
	}

	template <class ... Args>
	RenderBatch::Pointer allocate(Args&&... args) {
		RenderBatch::Pointer result = acquire();
		result->construct(std::forward<Args>(args)...);
		return result;
	}

private:
	void flush()
	{
		auto i = std::remove_if(_active.begin(), _active.end(), [](RenderBatch::Pointer& b) {
			bool notUsedAnywhere = (b->retainCount() == 1);
			
			if (notUsedAnywhere)
				b->clear();

			return b->retainCount() == 1;
		});

		if (i != _active.end())
		{
			_pool.insert(_pool.end(), i, _active.end());
			_active.erase(i, _active.end());
		}
	}

	RenderBatch::Pointer acquire()
	{
		flush();
		
		RenderBatch::Pointer result;
		if (_pool.empty())
		{ 
			result = RenderBatch::Pointer::create();
		}
		else
		{
			result = _pool.back();
			_pool.pop_back();
		}
		
		_active.emplace_back(result);
		return result;
	}

private:
	Vector<RenderBatch::Pointer> _active;
	Vector<RenderBatch::Pointer> _pool;
};

}
