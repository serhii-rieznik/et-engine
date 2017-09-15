/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/geometry/geometry.h>
#include <et/geometry/collision.h>
#include <et/core/conversion.h>
#include <et/rendering/base/renderbatch.h>

namespace et
{

RenderBatch::RenderBatch(const MaterialInstance::Pointer& m, const VertexStream::Pointer& v, uint32_t i, uint32_t ni) :
	_material(m), _vertexStream(v), _firstIndex(i), _numIndexes(ni)
{
}

void RenderBatch::calculateBoundingBox()
{
	ET_ASSERT(_vertexStorage.valid());
	ET_ASSERT(_indexArray.valid());

	if (_vertexStorage->hasAttributeWithType(VertexAttributeUsage::Position, DataType::Vec3))
	{
		vec3 minExtent = vec3(std::numeric_limits<float>::max());
		vec3 maxExtent = vec3(-std::numeric_limits<float>::max());
		const auto pos = _vertexStorage->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
		for (uint32_t i = _firstIndex, e = _firstIndex + _numIndexes; i < e; ++i)
		{
			const auto& v = pos[_indexArray->getIndex(i)];
			minExtent = minv(minExtent, v);
			maxExtent = maxv(maxExtent, v);
		}
		_boundingBox = BoundingBox(0.5f * (minExtent + maxExtent), 0.5f * (maxExtent - minExtent));
	}
	else
	{
		log::error("Unable to calculate support data - missing position attribute.");
	}
}

Dictionary RenderBatch::serialize() const
{
	Dictionary result;
	if (_vertexStorage.valid())
	{
		result.setStringForKey(kVertexStorageName, _vertexStorage->name());
	}
	if (_indexArray.valid())
	{
		result.setStringForKey(kIndexArrayName, _indexArray->name());
	}
	if (_material.valid())
	{
		result.setStringForKey(kMaterialName, _material->name());
	}
	if (_vertexStream.valid())
	{
		result.setStringForKey(kVertexArrayObjectName, _vertexStream->name());
	}
	result.setIntegerForKey(kStartIndex, _firstIndex);
	result.setIntegerForKey(kIndexesCount, _numIndexes);
	return result;
}

RayIntersection RenderBatch::intersectsLocalSpaceRay(const ray3d& ray) const
{
	RayIntersection result;

	if (_vertexStorage->hasAttributeWithType(VertexAttributeUsage::Position, DataType::Vec3) == false)
	{
		log::error("Unable to calculate intersection - missing position attribute.");
	}

	const auto pos = _vertexStorage->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
	uint32_t numTriangles = _numIndexes / 3;
	for (uint32_t t = 0; t < numTriangles; ++t)
	{
		auto i0 = _indexArray->getIndex(_firstIndex + 3 * t + 0);
		auto i1 = _indexArray->getIndex(_firstIndex + 3 * t + 1);
		auto i2 = _indexArray->getIndex(_firstIndex + 3 * t + 2);
		const vec3& p0 = pos[i0];
		const vec3& p1 = pos[i1];
		const vec3& p2 = pos[i2];
		vec3 ip;
		if (intersect::rayTriangle(ray, triangle(p0, p1, p2), &ip))
		{
			float d = (ip - ray.origin).dotSelf();
			if (d < result.time)
			{
				result.time = d;
				result.occurred = true;
			}
		}
	}

	return result;
}

RenderBatch* RenderBatch::duplicate() const
{
	RenderBatch* result = etCreateObject<RenderBatch>(_material, _vertexStream, _firstIndex, _numIndexes);
	result->setVertexStorage(_vertexStorage);
	result->setIndexArray(_indexArray);
	result->_boundingBox = _boundingBox;
	return result;
}

void RenderBatch::setMaterial(MaterialInstance::Pointer mat)
{
	_material = mat;
}

}
