/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/geometry/collision.h>

namespace et
{

class Frustum
{
public:
	void build(const mat4& inverseViewProjectionMatrix);
	bool containsBoundingBox(const BoundingBox& aabb) const;

private:
	std::array<plane, 6> _planes;
};

}
