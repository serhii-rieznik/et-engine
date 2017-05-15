/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/material.h>

namespace et
{
class Compute : public Object
{
public:
	ET_DECLARE_POINTER(Compute);

public:
	Compute(Material::Pointer m) 
		: _material(m->instance()) { }
	
	MaterialInstance::Pointer& material()
		{ return _material; }
	
	const MaterialInstance::Pointer& material() const
		{ return _material; }

private:
	MaterialInstance::Pointer _material;
};

}
