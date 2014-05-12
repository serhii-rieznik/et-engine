/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/camera/frustum.h>

using namespace et;

inline FrustumPlane operator ++(FrustumPlane p)
	{ return static_cast<FrustumPlane>(p + 1); }

Frustum::Frustum()
{
}

Frustum::Frustum(const mat4& mvp)
{
	_data[FrustumPlane_Right] = normalizePlane(vec4( mvp(3) - mvp(0), mvp(7) - mvp(4), mvp(11) - mvp( 8), mvp(15) - mvp(12) ));
	_data[FrustumPlane_Left] = normalizePlane(vec4( mvp(3) + mvp(0), mvp(7) + mvp(4), mvp(11) + mvp( 8), mvp(15) + mvp(12) ));
	_data[FrustumPlane_Bottom] = normalizePlane(vec4( mvp(3) + mvp(1), mvp(7) + mvp(5), mvp(11) + mvp( 9), mvp(15) + mvp(13) ));
	_data[FrustumPlane_Top]	= normalizePlane(vec4( mvp(3) - mvp(1), mvp(7) - mvp(5), mvp(11) - mvp( 9), mvp(15) - mvp(13) ));
	_data[FrustumPlane_Far] = normalizePlane(vec4( mvp(3) - mvp(2), mvp(7) - mvp(6), mvp(11) - mvp(10), mvp(15) - mvp(14) ));
	_data[FrustumPlane_Near] = normalizePlane(vec4( mvp(3) + mvp(2), mvp(7) + mvp(6), mvp(11) + mvp(10), mvp(15) + mvp(14) ));
}

bool Frustum::containSphere(const Sphere& sphere) const
{
	for (size_t p = FrustumPlane_Right; p < FrustumPlane_max; ++p)
	{
		if (_data[p].dot(vec4(sphere.center(), 1.0f)) + sphere.radius() <= 0.0f)
			return false;
	}

	return true;
}

bool Frustum::containAABB(const AABB& aabb) const
{
	for (FrustumPlane p = FrustumPlane_Right; p < FrustumPlane_max; ++p)
	{
		const vec4 plane = _data[p];

		if (aabb.corners[0].dot(plane) > 0) continue;
		if (aabb.corners[1].dot(plane) > 0) continue;
		if (aabb.corners[2].dot(plane) > 0) continue;
		if (aabb.corners[3].dot(plane) > 0) continue;
		if (aabb.corners[4].dot(plane) > 0) continue;
		if (aabb.corners[5].dot(plane) > 0) continue;
		if (aabb.corners[6].dot(plane) > 0) continue;
		if (aabb.corners[7].dot(plane) > 0) continue;

		return false;
	}

	return true;
}

bool Frustum::containOBB(const OBB&) const
{
	return false;
}
