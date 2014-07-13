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
	vec3 boxSize(50.0f, 35.0f, 60.0f);
	
	/*
	float scale = 12.0f;
	float offset = -boxSize.y / scale;
	
	ObjectsCache localCache;
	OBJLoader loader(rc, application().resolveFileName("models/gnome.obj"));
	auto container = loader.load(localCache, OBJLoader::Option_SupportMeshes);
	auto meshes = container->childrenOfType(s3d::ElementType_SupportMesh);
	for (s3d::SupportMesh::Pointer mesh : meshes)
	{
		for (const auto& tri : mesh->triangles())
		{
			vec3 a1(tri.v1().x, tri.v1().z + offset, tri.v1().y);
			vec3 a2(tri.v2().x, tri.v2().z + offset, tri.v2().y);
			vec3 a3(tri.v3().x, tri.v3().z + offset, tri.v3().y);
			objects.push_back(SceneObject(SceneObject::Class_Triangle, scale * a3,
				scale * a2, scale * a1, vec4(1.0f, 1.0f, 1.0f, 0.0f), vec4(0.0f)));
		}
	}
	*/
	
	float r0 = boxSize.length() / 4.0f;
	float r1 = 1.25f * r0;
	
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(1.00f, 0.5f, 0.25f, 0.0f), vec4(0.0f), 0.05f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere,
		vec4(-boxSize.x + r0, -boxSize.y + r0, -boxSize.z + r0, r0), 0));
	
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(0.25f, 0.5f, 1.00f, 0.0f), vec4(0.0f), 0.15f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere,
		vec4( boxSize.x - r1, -boxSize.y + r1, 0.0f, r1), 1));
	
	materials.push_back(SceneMaterial(vec4(0.0f), vec4(0.0f), vec4(33.0f), 0.0f));
	
	float r = 5.0f;
	vec3i lightsPerWall(3, 1, 4);
	vec3 dp = 2.0f * (boxSize - vec3(r)) / vector3ToFloat(lightsPerWall - vec3i(1));
	for (int i = 0; i < lightsPerWall.x; ++i)
	{
		objects.push_back(SceneObject(SceneObject::Class_Sphere,
			vec4(r - boxSize.x + dp.x * static_cast<float>(i), boxSize.y - r, -boxSize.z + r, r), 2));
		objects.push_back(SceneObject(SceneObject::Class_Sphere,
			vec4(r - boxSize.x + dp.x * static_cast<float>(i), boxSize.y - r,  boxSize.z - r, r), 2));
	}
	
	for (int i = 0; i < lightsPerWall.z; ++i)
	{
		objects.push_back(SceneObject(SceneObject::Class_Sphere,
			vec4(-boxSize.x + r, boxSize.y - r, -boxSize.z + dp.z * static_cast<float>(i), r), 2));
		objects.push_back(SceneObject(SceneObject::Class_Sphere,
			vec4( boxSize.x - r, boxSize.y - r, -boxSize.z + dp.z * static_cast<float>(i), r), 2));
	}
	
	int lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(1.0f), vec4(0.0f), 0.9f));
	
	// top
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, boxSize.y,  boxSize.z),
		vec3(-boxSize.x, boxSize.y, -boxSize.z),
		vec3( boxSize.x, boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, boxSize.y,  boxSize.z),
		vec3(-boxSize.x, boxSize.y,  boxSize.z),
		vec3( boxSize.x, boxSize.y, -boxSize.z), lastMaterial));
	
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(2.0f/3.0f), vec4(1.0f), vec4(0.0f), 0.9f));
	
	// bottom
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));

	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f, 1.0f/3.0f, 1.0f/3.0f, 0.0f), vec4(1.0f), vec4(0.0f), 0.9f));
	
	// left
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y, -boxSize.z), lastMaterial));
	
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f/3.0f, 1.0f, 1.0f/3.0f, 0.0f), vec4(1.0f), vec4(0.0f), 0.9f));
	
	// right
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y,  boxSize.z),
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z), lastMaterial));
	
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(0.75f), vec4(1.0f), vec4(0.0f), 0.9f));
	
	// back
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y, -boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x,  boxSize.y, -boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));
	
	// front
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y, boxSize.z),
		vec3( boxSize.x, -boxSize.y, boxSize.z),
		vec3(-boxSize.x, -boxSize.y, boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y, boxSize.z),
		vec3( boxSize.x,  boxSize.y, boxSize.z),
		vec3( boxSize.x, -boxSize.y, boxSize.z), lastMaterial));

	camera.perspectiveProjection(QUARTER_PI, 1.0f, 1.0f, 1024.0f);
}

const SceneObject& RaytraceScene::objectAtIndex(int i) const
{
	if ((i == Intersection::missingObject) || (i >= objects.size()))
		return emptyObject;
		
	return objects.at(i);
}

const SceneMaterial& RaytraceScene::materialAtIndex(int i) const
{
	if ((i == Intersection::missingObject) || (i >= materials.size()))
		return defaultMaterial;
	
	return materials.at(i);
}