//
//  SceneIntersection.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 16/11/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include "SceneIntersection.h"

using namespace et;
using namespace rt;

bool rt::rayAABB(const et::ray3d& r, const et::AABB& box)
{
	vec3 inv_direction(1.0f / r.direction.x, 1.0f / r.direction.y, 1.0f / r.direction.z);
	vec3i r_sign(inv_direction.x < 0.0f ? 1 : 0, inv_direction.y < 0.0f ? 1 : 0, inv_direction.z < 0.0f ? 1 : 0);
	
	vec3 parameters[2] = { box.minVertex(), box.maxVertex() };
	
	float txmin = (parameters[r_sign.x].x - r.origin.x) * inv_direction.x;
	float tymin = (parameters[r_sign.y].y - r.origin.y) * inv_direction.y;
	float txmax = (parameters[1 - r_sign.x].x - r.origin.x) * inv_direction.x;
	float tymax = (parameters[1 - r_sign.y].y - r.origin.y) * inv_direction.y;
	
	if ((txmin < tymax) && (tymin < txmax))
	{
		if (tymin > txmin) txmin = tymin;
		if (tymax < txmax) txmax = tymax;
		
		float tzmin = (parameters[r_sign.z].z - r.origin.z) * inv_direction.z;
		float tzmax = (parameters[1 - r_sign.z].z - r.origin.z) * inv_direction.z;

		if ((txmin < tzmax) && (tzmin < txmax))
			return true;
	}
	
	return false;
}
