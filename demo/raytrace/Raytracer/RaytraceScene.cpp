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
	{
		vec3 p1;
		vec3 p2;
		if (intersect::raySphere(ray, Sphere(equation.xyz(), equation.w), &p1, &p2))
		{
			point = (ray.origin - p1).dotSelf() > 0.01f ? p1 : p2;
			return true;
		}
	}
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
	
	float r0 = 25.0f;
	float r1 = 10.0f;
	float r2 = 15.0f;
	
	materials.push_back(SceneMaterial(vec4(0.5f, 1.0f, 0.5f, 1.0f), vec4(1.0f), vec4(0.0f), 0.25f, 2.48417f));
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(1.0f), vec4(0.0f), 0.00f, 2.48417f));
	materials.push_back(SceneMaterial(vec4(0.5f, 0.5f, 1.0f, 1.0f), vec4(1.0f), vec4(0.0f), 0.10f, 2.48417f));
	
	materials.push_back(SceneMaterial(vec4(1.0f, 0.75f, 0.5f, 1.0f), vec4(0.5f, 0.75f, 1.0f, 1.0f), vec4(0.0f), 0.05f, 2.48417f));

	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(-boxSize.x +        r1, -boxSize.y + r1, -boxSize.z + 2.0f * r1, r1), 0));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(-boxSize.x +        r1, -boxSize.y + r1,  boxSize.z - 2.0f * r1, r1), 2));

	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(-boxSize.x + 1.75f * r2, -boxSize.y + r2,	0.0f, r2), 1));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4( boxSize.x - 1.25f * r0, -boxSize.y + r0, 0.0f, r0), 3));
	
	int lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(1.0f), vec4(0.0f), 1.0f));
	
	// top
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, boxSize.y,  boxSize.z),
		vec3(-boxSize.x, boxSize.y, -boxSize.z),
		vec3( boxSize.x, boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, boxSize.y,  boxSize.z),
		vec3(-boxSize.x, boxSize.y,  boxSize.z),
		vec3( boxSize.x, boxSize.y, -boxSize.z), lastMaterial));
	
	// top light
	float s = 1.0f / 4.0f;
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(1.0f), vec4(50.0f), 1.0f));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-s * boxSize.x, 0.99f * boxSize.y,  s * boxSize.z),
		vec3(-s * boxSize.x, 0.99f * boxSize.y, -s * boxSize.z),
		vec3( s * boxSize.x, 0.99f * boxSize.y, -s * boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( s * boxSize.x, 0.99f * boxSize.y,  s * boxSize.z),
		vec3(-s * boxSize.x, 0.99f * boxSize.y,  s * boxSize.z),
		vec3( s * boxSize.x, 0.99f * boxSize.y, -s * boxSize.z), lastMaterial));
	
	// bottom
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(2.0f/3.0f), vec4(1.0f), vec4(0.0f), 1.0f));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));

	// left
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f, 1.0f/3.0f, 1.0f/3.0f, 0.0f), vec4(1.0f), vec4(0.0f), 1.0f));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y,  boxSize.z),
		vec3( boxSize.x,  boxSize.y, -boxSize.z), lastMaterial));
	
	// right
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f/3.0f, 1.0f, 1.0f/3.0f, 0.0f), vec4(1.0f), vec4(0.0f), 1.0f));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y,  boxSize.z),
		vec3(-boxSize.x, -boxSize.y,  boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z), lastMaterial));
	
	// back
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(0.75f), vec4(1.0f), vec4(0.0f), 1.0f));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x,  boxSize.y, -boxSize.z),
		vec3(-boxSize.x, -boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x,  boxSize.y, -boxSize.z),
		vec3(-boxSize.x,  boxSize.y, -boxSize.z),
		vec3( boxSize.x, -boxSize.y, -boxSize.z), lastMaterial));
	
	// front
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(0.75f), vec4(1.0f), vec4(0.0f), 1.0f));
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
	if ((i == Intersection::missingObject) || (i >= static_cast<int>(objects.size())))
		return emptyObject;
		
	return objects.at(i);
}

const SceneMaterial& RaytraceScene::materialAtIndex(int i) const
{
	if ((i == Intersection::missingObject) || (i >=  static_cast<int>(materials.size())))
		return defaultMaterial;
	
	return materials.at(i);
}