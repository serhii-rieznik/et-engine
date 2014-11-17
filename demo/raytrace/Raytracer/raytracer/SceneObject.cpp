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
//	if (rayAABB(ray, _boundingBox))
	{
		for (const auto& t : _triangles)
		{
			if (intersect::rayTriangle(ray, t, &point))
			{
				normal = t.normalizedNormal();
				return true;
			}
		}
	}
	return false;
}

/*
struct SceneObject
{
 enum Class
 {
	 Class_None,
	 Class_Sphere,
	 Class_Plane,
	 Class_Triangle,
	 Class_Mesh,
 };
 
 Class objectClass = Class_None;
 
 et::triangle tri;
 et::vec4 equation;
 et::DataStorage<et::triangle> mesh;
 
 size_t materialId = MissingObjectIndex;
 size_t objectId = 0;
 
public:
 SceneObject()
 { }
 
 SceneObject(Class cls, const et::vec4& eq, size_t mat) :
 objectClass(cls), equation(eq), materialId(mat) { }
 
 SceneObject(const et::triangle& t, size_t mat) :
 objectClass(Class_Triangle), tri(t), materialId(mat) { }
 
 SceneObject(const et::vec3& p1, const et::vec3& p2, const et::vec3& p3, size_t mat) :
 objectClass(Class_Triangle), tri(p1, p2, p3), materialId(mat) { }
 
 SceneObject(const et::s3d::SupportMesh::Pointer& m, size_t mat);
 
public:
 bool intersectsRay(const et::ray3d&, et::vec3& point, et::vec3& normal) const;
 et::vec3 normalFromPoint(const et::vec3&) const;
};


bool SceneObject::intersectsRay(const et::ray3d& ray, et::vec3& point, et::vec3& normal) const
{
	if (objectClass == Class_Sphere)
	{
		vec3 p1;
		vec3 p2;
		if (intersect::raySphere(ray, Sphere(equation.xyz(), equation.w), &p1, &p2))
		{
			point = (ray.origin - p1).dotSelf() > 0.01f ? p1 : p2;
			normal = (point - equation.xyz()).normalized();
			return true;
		}
	}
	else if (objectClass == Class_Plane)
	{
		if (intersect::rayPlane(ray, plane(equation), &point))
		{
			normal = equation.xyz();
			return true;
		}
		
	}
	else if (objectClass == Class_Triangle)
	{
		if (intersect::rayTriangle(ray, tri, &point))
		{
			if ((ray.origin - point).dotSelf() < 0.001f)
				return false;
			
			normal = tri.normalizedNormal();
			return true;
		}
	}
	else if (objectClass == Class_Mesh)
	{
	}
	
	return false;
}
*/
