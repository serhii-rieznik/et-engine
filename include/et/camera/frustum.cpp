/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/camera/frustum.h>

namespace et
{

void Frustum::build(const mat4& invVP)
{
	float zNear = Camera::zeroClipRange ? 0.0f : -1.0f;
	float zFar = 1.0f;
	
	_corners[0] = invVP * vec3(-1.0f, -1.0f, zNear);
	_corners[1] = invVP * vec3( 1.0f, -1.0f, zNear);
	_corners[2] = invVP * vec3(-1.0f,  1.0f, zNear);
	_corners[3] = invVP * vec3( 1.0f,  1.0f, zNear);
	_corners[4] = invVP * vec3(-1.0f, -1.0f, zFar);
	_corners[5] = invVP * vec3( 1.0f, -1.0f, zFar);
	_corners[6] = invVP * vec3(-1.0f,  1.0f, zFar);
	_corners[7] = invVP * vec3( 1.0f,  1.0f, zFar);

	_planes[0] = plane(triangle(_corners[0], _corners[2], _corners[1]));
	_planes[1] = plane(triangle(_corners[4], _corners[5], _corners[6]));
	_planes[2] = plane(triangle(_corners[0], _corners[1], _corners[4]));
	_planes[3] = plane(triangle(_corners[2], _corners[6], _corners[3]));
	_planes[4] = plane(triangle(_corners[0], _corners[4], _corners[2]));
	_planes[5] = plane(triangle(_corners[1], _corners[3], _corners[5]));
}

bool Frustum::containsBoundingBox(const BoundingBox& aabb) const
{
	BoundingBox::Corners corners;
	aabb.calculateCorners(corners);

	for (const plane& frustumPlane : _planes)
	{
		uint32_t outCorners = 0;
		for (const vec3& corner : corners)
		{
			if (dot(frustumPlane.equation.xyz(), corner) > frustumPlane.equation.w)
				++outCorners;
		}
		if (outCorners == corners.size())
			return false;
	}

	// TODO : check behind
	// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm

	return true;
}

}
