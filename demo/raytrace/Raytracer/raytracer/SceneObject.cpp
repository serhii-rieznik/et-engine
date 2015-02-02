//
//  SceneObject.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 16/11/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include "SceneObject.h"

using namespace et;
using namespace rt;

MeshObject::MeshObject(size_t first, size_t count, const SceneTriangleList& tris) :
	SceneObject(0), _firstTriangle(first), _numTriangles(count), _triangles(tris)
{
	buildBoundingBox();
}

void MeshObject::buildBoundingBox()
{
	vec3 maxVertex = _triangles[_firstTriangle].tri.v1();
	vec3 minVertex = _triangles[_firstTriangle].tri.v1();
	const SceneTriangle* t = _triangles.element_ptr(_firstTriangle);
	const SceneTriangle* last = t + _numTriangles;
	
	while (t != last)
	{
		maxVertex = maxv(maxVertex, t->tri.v1());
		maxVertex = maxv(maxVertex, t->tri.v2());
		maxVertex = maxv(maxVertex, t->tri.v3());
		minVertex = minv(minVertex, t->tri.v1());
		minVertex = minv(minVertex, t->tri.v2());
		minVertex = minv(minVertex, t->tri.v3());
		++t;
	}
	
	_boundingBox = AABB(0.5f * (minVertex + maxVertex), 0.5f * (maxVertex - minVertex));
}

bool MeshObject::intersects(const et::ray3d& ray, et::vec3& point, et::vec3& normal, size_t& matIndex)
{
	if (!rayAABB(ray, _boundingBox)) return false;
	
	float minDistance = std::numeric_limits<float>::max();
	
	const SceneTriangle* t = _triangles.element_ptr(_firstTriangle);
	const SceneTriangle* last = t + _numTriangles;
	
	vec3 localHitPoint;
	const SceneTriangle* hitTriangle = nullptr;
	while (t != last)
	{
		if (intersect::rayTriangleTwoSided(ray, t->tri, &localHitPoint))
		{
			float distanceToLocalPoint = (localHitPoint - ray.origin).dotSelf();
			if (distanceToLocalPoint < minDistance)
			{
				hitTriangle = t;
				point = localHitPoint;
				minDistance = distanceToLocalPoint;
			}
		}
		++t;
	}
	
	if (hitTriangle != nullptr)
	{
		matIndex = hitTriangle->materialIndex;
		normal = hitTriangle->tri.interpolatedNormal(hitTriangle->tri.barycentric(point));
		return true;
	}
	
	return false;
}
