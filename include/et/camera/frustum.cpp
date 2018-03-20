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
	return true;

	const BoundingBox::Corners& corners = aabb.corners();

	for (const plane& frustumPlane : _planes)
	{
		const vec3& eq = frustumPlane.equation.xyz();
		float w = frustumPlane.equation.w;
		
		uint32_t outCorners = 0;
		outCorners += (dot(eq, corners[0]) > w) ? 1u : 0u;
		outCorners += (dot(eq, corners[1]) > w) ? 1u : 0u;
		outCorners += (dot(eq, corners[2]) > w) ? 1u : 0u;
		outCorners += (dot(eq, corners[3]) > w) ? 1u : 0u;
		outCorners += (dot(eq, corners[4]) > w) ? 1u : 0u;
		outCorners += (dot(eq, corners[5]) > w) ? 1u : 0u;
		outCorners += (dot(eq, corners[6]) > w) ? 1u : 0u;
		outCorners += (dot(eq, corners[7]) > w) ? 1u : 0u;

		if (outCorners == 8)
			return false;
	}

	// TODO : check behind
	// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm

	return true;
}

}
