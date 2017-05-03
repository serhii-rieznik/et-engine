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
}
