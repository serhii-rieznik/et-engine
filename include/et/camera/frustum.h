/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
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

	typedef StaticDataStorage<vec4, FrustumPlane_max> FrustumData;

	class Frustum
	{
	public:
		Frustum();
		Frustum(const mat4& mvpMatrix);

		bool containSphere(const Sphere& sphere) const;
		bool containAABB(const AABB& aabb) const;
		bool containOBB(const OBB& obb) const;

	private:
		FrustumData _data;
	};

}
