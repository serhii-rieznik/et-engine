/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/geometry/geometry.h>
#include <et/core/conversion.h>
#include <et/rendering/renderbatch.h>

using namespace et;

RenderBatch::RenderBatch(const Material::Pointer& m, const VertexArrayObject::Pointer& v, const mat4& transform) :
	_material(m), _vao(v), _firstIndex(0), _numIndexes(v->indexBuffer()->size()), _transformation(transform)
{
	
}

RenderBatch::RenderBatch(const Material::Pointer& m, const VertexArrayObject::Pointer& v,
	const mat4& transform, uint32_t i, uint32_t ni) : _material(m), _vao(v), _firstIndex(i), _numIndexes(ni),
	_transformation(transform)
{
	
}

void RenderBatch::calculateBoundingBox()
{
	ET_ASSERT(_vertexStorage.valid());
	ET_ASSERT(_indexArray.valid());
	
	if (_vertexStorage->hasAttributeWithType(VertexAttributeUsage::Position, DataType::Vec3))
	{
		_minExtent = vec3(std::numeric_limits<float>::max());
		_maxExtent = vec3(-std::numeric_limits<float>::max());
		const auto pos = _vertexStorage->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
		for (uint32_t i = _firstIndex, e = _firstIndex + _numIndexes; i < e; ++i)
		{
			const auto& v = pos[_indexArray->getIndex(i)];
			_minExtent = minv(_minExtent, v);
			_maxExtent = maxv(_maxExtent, v);
		}
		_boundingBox = BoundingBox(0.5f * (_minExtent + _maxExtent), 0.5f * (_maxExtent - _minExtent));
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
	if (_vao.valid())
	{
		result.setStringForKey(kVertexArrayObjectName, _vao->name());
	}
	result.setIntegerForKey(kStartIndex, _firstIndex);
	result.setIntegerForKey(kIndexesCount, _numIndexes);
	
	/*
	ArrayValue transform;
	transform->content.resize(4);
	transform->content[0] = vec4ToArray(_transformation[0]);
	transform->content[1] = vec4ToArray(_transformation[1]);
	transform->content[2] = vec4ToArray(_transformation[2]);
	transform->content[3] = vec4ToArray(_transformation[3]);
	result.setArrayForKey(kTransform, transform);
	*/
	
	return result;
}
