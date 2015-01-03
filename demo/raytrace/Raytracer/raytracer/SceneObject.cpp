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

MeshObject::MeshObject(const et::s3d::SupportMesh::Pointer& m, size_t mat) :
	SceneObject(mat)
{
	_triangles = m->triangles();
	_boundingSphere = Sphere(m->center(), m->radius());
	_boundingBox = AABB(m->center(), m->size());
}

bool MeshObject::intersects(const et::ray3d& ray, et::vec3& point, et::vec3& normal)
{
	if (!rayAABB(ray, _boundingBox)) return false;
	
	bool hit = false;
	float minDistance = std::numeric_limits<float>::max();
	
	for (const auto& t : _triangles)
	{
		vec3 localPoint;
		
		if (intersect::rayTriangleTwoSided(ray, t, &localPoint))
		{
			float distanceToLocalPoint = (localPoint - ray.origin).dotSelf();
			if (distanceToLocalPoint < minDistance)
			{
				hit = true;
				point = localPoint;
				normal = t.normalizedNormal();
				minDistance = distanceToLocalPoint;
			}
		}
	}
	
	return hit;
}
