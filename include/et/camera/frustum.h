/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/collision/collision.h>

namespace et
{
	enum FrustumPlane
	{
		FrustumPlane_Right,
		FrustumPlane_Left,
		FrustumPlane_Bottom,
		FrustumPlane_Top,
		FrustumPlane_Far,
		FrustumPlane_Near,
		FrustumPlane_max
	};

	class Frustum
	{
	public:
		Frustum();
		Frustum(const mat4& mvpMatrix);

		bool containsSphere(const Sphere& sphere) const;
		bool containsAABB(const AABB& aabb) const;
		bool containsOBB(const OBB& obb) const;

	private:
		StaticDataStorage<vec4, FrustumPlane_max> _planes;
		StaticDataStorage<vec4, 8> _corners;
	};

}
