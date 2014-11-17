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

//bool Box::intersect(const Ray &r, float t0, float t1) const

bool rt::rayAABB(const et::ray3d& r, const et::AABB& box)
{
	vec3 inv_direction = vec3(1.0f) / r.direction;
	vec3i r_sign(inv_direction.x < 0.0f ? 1 : 0, inv_direction.y < 0.0f ? 1 : 0, inv_direction.z < 0.0f ? 1 : 0);
	vec3 parameters[2] = { box.minVertex(), box.maxVertex() };
	
	float tmin = (parameters[r_sign[0]].x - r.origin.x) * inv_direction.x;
	float tmax = (parameters[1 - r_sign[0]].x - r.origin.x) * inv_direction.x;
	float tymin = (parameters[r_sign[1]].y - r.origin.y) * inv_direction.y;
	float tymax = (parameters[1 - r_sign[1]].y - r.origin.y) * inv_direction.y;
	
	if ((tmin > tymax) || (tymin > tmax))
		return false;
	
	if (tymin > tmin)
		tmin = tymin;
	
	if (tymax < tmax)
		tmax = tymax;
	
	float tzmin = (parameters[r_sign[2]].z - r.origin.z) * inv_direction.z;
	float tzmax = (parameters[1 - r_sign[2]].z - r.origin.z) * inv_direction.z;
	
	if ((tmin > tzmax) || (tzmin > tmax))
		return false;
	
	if (tzmin > tmin)
		tmin = tzmin;
	
	if (tzmax < tmax)
		tmax = tzmax;
	
	return true;
}
