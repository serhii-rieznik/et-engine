/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
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
	for (size_t p = FrustumPlane_Right; p < FrustumPlane_max; ++p)
	{
		const vec4& plane = _data[p];
		
		bool sholdReturn = true;
		
		for (size_t c = AABBCorner_First; c < AABBCorner_max; ++c)
		{
			if (dot(vec4(aabb.corners[0], 1.0f), plane) > 0.0f)
			{
				sholdReturn = false;
				break;
			}
		}
		
		if (sholdReturn)
			return false;
	}

	return true;
}

bool Frustum::containOBB(const OBB&) const
{
	ET_FAIL("Not implemented")
	return false;
}
