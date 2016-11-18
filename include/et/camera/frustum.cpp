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
	vec3 c000 = invVP * vec3(-1.0f, -1.0f, -1.0f);
	vec3 c100 = invVP * vec3( 1.0f, -1.0f, -1.0f);
	vec3 c010 = invVP * vec3(-1.0f,  1.0f, -1.0f);
	vec3 c110 = invVP * vec3( 1.0f,  1.0f, -1.0f);	
	vec3 c001 = invVP * vec3(-1.0f, -1.0f,  1.0f);
	vec3 c101 = invVP * vec3( 1.0f, -1.0f,  1.0f);
	vec3 c011 = invVP * vec3(-1.0f,  1.0f,  1.0f);
	vec3 c111 = invVP * vec3( 1.0f,  1.0f,  1.0f);
	planes[0] = plane(triangle(c000, c010, c100));
	planes[1] = plane(triangle(c001, c101, c011));
	planes[2] = plane(triangle(c000, c100, c001));
	planes[3] = plane(triangle(c010, c011, c110));
	planes[4] = plane(triangle(c000, c001, c010));
	planes[5] = plane(triangle(c100, c110, c101));
}

bool Frustum::containsBoundingBox(const BoundingBox& aabb) const
{
	BoundingBox::Corners corners;
	aabb.calculateCorners(corners);

	for (const plane& frustumPlane : planes)
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
