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

bool rt::rayAABB(et::ray3d r, const et::AABB& box)
{
	r.direction.x = 1.0f / r.direction.x;
	r.direction.y = 1.0f / r.direction.y;
	r.direction.z = 1.0f / r.direction.z;
	
	int r_sign_x = (r.direction.x < 0.0f ? 1 : 0);
	int r_sign_y = (r.direction.y < 0.0f ? 1 : 0);
	
	vec3 parameters[2] = { box.minVertex(), box.maxVertex() };
	
	float txmin = (parameters[r_sign_x].x - r.origin.x) * r.direction.x;
	float tymin = (parameters[r_sign_y].y - r.origin.y) * r.direction.y;
	
	float txmax = (parameters[1 - r_sign_x].x - r.origin.x) * r.direction.x;
	float tymax = (parameters[1 - r_sign_y].y - r.origin.y) * r.direction.y;
	
	if ((txmin < tymax) && (tymin < txmax))
	{
		if (tymin > txmin) txmin = tymin;
		if (tymax < txmax) txmax = tymax;
		
		int r_sign_z = (r.direction.z < 0.0f ? 1 : 0);
		
		float tzmin = (parameters[r_sign_z].z - r.origin.z) * r.direction.z;
		float tzmax = (parameters[1 - r_sign_z].z - r.origin.z) * r.direction.z;

		if ((txmin < tzmax) && (tzmin < txmax))
			return true;
	}
	
	return false;
}
