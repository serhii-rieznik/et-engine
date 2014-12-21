/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
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
