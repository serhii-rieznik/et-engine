/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/renderbatch.h>

using namespace et;

RenderBatch::RenderBatch(const Material::Pointer& m, const VertexArrayObject::Pointer& v, const mat4& transform) :
	_material(m), _vao(v), _firstIndex(0), _numIndexes(v->indexBuffer()->size()), _transformation(transform)
{
	
}

RenderBatch::RenderBatch(const Material::Pointer& m, const VertexArrayObject::Pointer& v, const mat4& transform, uint32_t i, uint32_t ni) :
	_material(m), _vao(v), _firstIndex(i), _numIndexes(ni), _transformation(transform)
{
	
}