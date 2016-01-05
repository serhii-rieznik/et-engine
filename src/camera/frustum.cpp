/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/camera/frustum.h>

using namespace et;

Frustum::Frustum()
{
}

Frustum::Frustum(const mat4& mvp)
{
	build(mvp);
}

void Frustum::build(const mat4& mvp)
{
	_planes[FrustumPlane_Right] = normalizePlane(vec4(mvp(3) - mvp(0), mvp(7) - mvp(4), mvp(11) - mvp(8), mvp(15) - mvp(12)));
	_planes[FrustumPlane_Left] = normalizePlane(vec4(mvp(3) + mvp(0), mvp(7) + mvp(4), mvp(11) + mvp(8), mvp(15) + mvp(12)));
	_planes[FrustumPlane_Bottom] = normalizePlane(vec4(mvp(3) + mvp(1), mvp(7) + mvp(5), mvp(11) + mvp(9), mvp(15) + mvp(13)));
	_planes[FrustumPlane_Top]	= normalizePlane(vec4(mvp(3) - mvp(1), mvp(7) - mvp(5), mvp(11) - mvp(9), mvp(15) - mvp(13)));
	_planes[FrustumPlane_Far] = normalizePlane(vec4(mvp(3) - mvp(2), mvp(7) - mvp(6), mvp(11) - mvp(10), mvp(15) - mvp(14)));
	_planes[FrustumPlane_Near] = normalizePlane(vec4(mvp(3) + mvp(2), mvp(7) + mvp(6), mvp(11) + mvp(10), mvp(15) + mvp(14)));
	
	mat4 invMVP = mvp.inverse();
	
	_corners[0] = invMVP * vec4(-1.0f, -1.0f, -1.0f, 1.0f);
	_corners[1] = invMVP * vec4(-1.0f, -1.0f,  1.0f, 1.0f);
	_corners[2] = invMVP * vec4(-1.0f,  1.0f, -1.0f, 1.0f);
	_corners[3] = invMVP * vec4(-1.0f,  1.0f,  1.0f, 1.0f);
	_corners[4] = invMVP * vec4( 1.0f, -1.0f, -1.0f, 1.0f);
	_corners[5] = invMVP * vec4( 1.0f, -1.0f,  1.0f, 1.0f);
	_corners[6] = invMVP * vec4( 1.0f,  1.0f, -1.0f, 1.0f);
	_corners[7] = invMVP * vec4( 1.0f,  1.0f,  1.0f, 1.0f);
}

bool Frustum::containsSphere(const Sphere& sphere) const
{
	for (uint32_t p = FrustumPlane_Right; p < FrustumPlane_max; ++p)
	{
		if (_planes[p].dot(vec4(sphere.center(), 1.0f)) + sphere.radius() <= 0.0f)
			return false;
	}

	return true;
}

bool Frustum::containsAABB(const AABB& aabb) const
{
	for (const auto& frustumPlane : _planes)
	{
		if (dot(aabb.center, frustumPlane.xyz()) + 2.0 * dot(aabb.halfDimension, absv(frustumPlane.xyz())) < -frustumPlane.w)
			return false;
	}
	
	return true;
}

bool Frustum::containsOBB(const OBB&) const
{
	ET_FAIL("Not implemented")
	return false;
}
