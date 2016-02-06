/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/geometry.h>
#include <et/rendering/material.h>
#include <et/rendering/vertexarrayobject.h>
#include <et/rendering/vertexstorage.h>
#include <et/rendering/indexarray.h>

namespace et
{
	class RenderBatch : public et::Shared
	{
	public:
		ET_DECLARE_POINTER(RenderBatch)
		
	public:
		RenderBatch() = default;
		RenderBatch(const Material::Pointer&, const VertexArrayObject::Pointer&, const mat4& transform = identityMatrix);
		RenderBatch(const Material::Pointer&, const VertexArrayObject::Pointer&, const mat4& transform, uint32_t, uint32_t);

		Material::Pointer& material()
			{ return _material; }
		
		const Material::Pointer& material() const
			{ return _material; }
		
		VertexArrayObject::Pointer& vao()
			{ return _vao; }
		const VertexArrayObject::Pointer& vao() const
			{ return _vao; }
		
		uint32_t firstIndex() const
			{ return _firstIndex; }
		uint32_t numIndexes() const
			{ return _numIndexes; }
		
		const mat4& transformation() const
			{ return _transformation; }
		void setTransformation(const mat4& m)
			{ _transformation = m ; }
		
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
		const vec3& maxExtent() const
			{ return _maxExtent; }
		const vec3& minExtent() const
			{ return _minExtent; }

		const BoundingBox& transformedBoundingBox();
		
		bool intersectsLocalSpaceRay(const ray3d&, vec3& intersection);
		
		Dictionary serialize() const;
		
		void makeMaterialSnapshot();
		
		uint64_t materialSnapshot() const
			{ return _materialSnapshot; }
		
		RenderBatch* duplicate() const;
		
	private:
		Material::Pointer _material;
		VertexArrayObject::Pointer _vao;
		VertexStorage::Pointer _vertexStorage;
		IndexArray::Pointer _indexArray;
		mat4 _transformation = identityMatrix;
		vec3 _minExtent;
		vec3 _maxExtent;
		BoundingBox _boundingBox;
		BoundingBox _transformedBoudingBox;
		uint32_t _firstIndex = 0;
		uint32_t _numIndexes = 0;
		uint64_t _materialSnapshot = uint64_t(-1);
		bool _transformedBoxValid = false;
	};
}
