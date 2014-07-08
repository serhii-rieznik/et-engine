//
//  RaytraceScene.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/app/application.h>
#include <et/models/objloader.h>
#include "RaytraceScene.h"

using namespace rt;
using namespace et;

bool SceneObject::intersectsRay(const et::ray3d& ray, et::vec3& point) const
{
	if (objectClass == Class_Sphere)
		return intersect::raySphere(ray, Sphere(equation.xyz(), equation.w), &point);
	else if (objectClass == Class_Plane)
		return intersect::rayPlane(ray, plane(equation), &point);
	else if (objectClass == Class_Triangle)
		return intersect::rayTriangle(ray, tri, &point);
	
	return false;
}

et::vec3 SceneObject::normalFromPoint(const vec3& pt) const
{
	if (objectClass == Class_Plane)
		return equation.xyz();
	else if (objectClass == Class_Sphere)
		return normalize(pt - equation.xyz());
	else if (objectClass == Class_Triangle)
		return tri.normalizedNormal();
		
	return vec3(0.0f);
}

void RaytraceScene::load(et::RenderContext* rc)
{
	vec3 boxSize(50.0f, 30.0f, 60.0f);
	
	float scale = 9.0f;
	float offset = -boxSize.y / scale;
	
	ObjectsCache localCache;
	OBJLoader loader(rc, application().resolveFileName("models/gnome.obj"));
	auto container = loader.load(localCache, OBJLoader::Option_SupportMeshes);
	auto meshes = container->childrenOfType(s3d::ElementType_SupportMesh);
	for (s3d::SupportMesh::Pointer mesh : meshes)
	{
		log::info("%s", mesh->name().c_str());
		for (const auto& tri : mesh->triangles())
		{
			vec3 a1(tri.v1().x, tri.v1().z + offset, tri.v1().y);
			vec3 a2(tri.v2().x, tri.v2().z + offset, tri.v2().y);
			vec3 a3(tri.v3().x, tri.v3().z + offset, tri.v3().y);
			
			objects.push_back(SceneObject(SceneObject::Class_Triangle, scale * a3,
				scale * a2, scale * a1, vec4(1.0f), vec4(0.0f)));
		}
	}
	
	/*
	float r = 15.0f;
	 
	// sphere
	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(boxSize.x - r, -boxSize.y + r, boxSize.z - r, r),
		vec4(0.25f, 0.5f, 1.0f, 0.0f), vec4(0.0f)));

	// sphere
	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(r - boxSize.x, -boxSize.y + r, 0.0f, r),
		vec4(1.0f, 0.5f, 0.25f, 0.0f), vec4(0.0f)));

	// focus spheres
	float h = 5.0f;
	float z = -boxSize.z + h;
	float x = boxSize.x - h;
	while (z < boxSize.z)
	{
		objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(x, -boxSize.y + h, z, h),
			vec4(1.0f, 1.0f, 1.0f, 0.0f), vec4(0.0f)));
		z += 2.25f * h;
		x -= (boxSize.z / boxSize.x) * h;
	}
	*/
	float l = 3.0f;
	int i = 0;
	float z = -boxSize.z + 2.0f * l;
	float dz = 3.0f * (boxSize.z - 2.0f * l) / 10.0f;
	float dx = 3.0f * (boxSize.x - 2.0f * l) / 10.0f;
	while (z < boxSize.z - 2.0f * l)
	{
		float x = -boxSize.x + l * (2.0f + ((i++ % 2 == 0) ? 0.0f : 0.5f * dx));
		
		while (x < boxSize.x - 2.0f * l)
		{
			objects.push_back(SceneObject(SceneObject::Class_Sphere,vec4(x, boxSize.y, z, l), vec4(0.0f),
				5.0f * vec4(randomFloat(3.0f, 10.0f), randomFloat(3.0f, 10.0f), randomFloat(3.0f, 10.0f), 1.0f)));
			
			x += dx;
		}
		z += dz;
	}
	// */
	
	// top
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, boxSize.y,  boxSize.z),
		vec3(-boxSize.x, boxSize.y, -boxSize.z),
		vec3( boxSize.x, boxSize.y, -boxSize.z), vec4(1.0f, 0.5f), vec4(0.0f)));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, boxSize.y,  boxSize.z),
		vec3(-boxSize.x, boxSize.y,  boxSize.z),
		vec3( boxSize.x, boxSize.y, -boxSize.z), vec4(1.0f, 0.5f), vec4(0.0f)));
	
	// bottom
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z), vec4(2.0/3.0f, 0.25f), vec4(0.0f)));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), vec4(2.0/3.0f, 0.25f), vec4(0.0f)));

	// left
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), vec4(1.0f, 0.5f, 0.5f, 0.15f), vec4(0.0f)));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y, -boxSize.z), vec4(1.0f, 0.5f, 0.5f, 0.15f), vec4(0.0f)));
	
	// right
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z), vec4(0.5f, 1.0f, 0.5f, 0.15f), vec4(0.0f)));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y,  boxSize.z),
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z), vec4(0.5f, 1.0f, 0.5f, 0.15f), vec4(0.0f)));
	
	// back
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y, -boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), vec4(0.75f, 0.2f), vec4(0.0f)));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x,  boxSize.y, -boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), vec4(0.75f, 0.2f), vec4(0.0f)));
	
	// front
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y, boxSize.z),
		vec3( boxSize.x, -boxSize.y, boxSize.z),
		vec3(-boxSize.x, -boxSize.y, boxSize.z), vec4(0.75f, 0.2f), vec4(0.0f)));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y, boxSize.z),
		vec3( boxSize.x,  boxSize.y, boxSize.z),
		vec3( boxSize.x, -boxSize.y, boxSize.z), vec4(0.75f, 0.2f), vec4(0.0f)));

	camera.perspectiveProjection(QUARTER_PI, 1.0f, 1.0f, 1024.0f);
}

const SceneObject& RaytraceScene::objectAtIndex(int i) const
{
	if ((i == Intersection::missingObject) || (i >= objects.size()))
		return emptyObject;
		
	return objects.at(i);
}
