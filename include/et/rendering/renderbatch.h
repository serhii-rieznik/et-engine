/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/material.h>
#include <et/rendering/vertexarrayobject.h>

namespace et
{
	class RenderBatch : public et::Shared
	{
	public:
		ET_DECLARE_POINTER(RenderBatch)
		
	public:
		RenderBatch() = default;
		RenderBatch(const Material::Pointer&, const VertexArrayObject&);
		RenderBatch(const Material::Pointer&, const VertexArrayObject&, uint32_t, uint32_t);

		const Material::Pointer& material() const
			{ return _material; }
		
		const VertexArrayObject& data() const
			{ return _data; }
		
		const uint32_t firstIndex() const
			{ return _firstIndex; }
		
		const uint32_t numIndexes() const
			{ return _numIndexes; }
		
	private:
		Material::Pointer _material;
		VertexArrayObject _data;
		uint32_t _firstIndex = 0;
		uint32_t _numIndexes = 0;
	};
}
