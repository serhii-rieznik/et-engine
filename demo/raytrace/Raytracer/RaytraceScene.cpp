//
//  RaytraceScene.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include "RaytraceScene.h"

using namespace rt;
using namespace et;

bool SceneObject::intersectsRay(const et::ray3d& ray, et::vec3& point) const
{
	if ((objectClass == Class_Sphere) || (objectClass == Class_SphericalLight))
		return intersect::raySphere(ray, Sphere(equation.xyz(), equation.w), &point);
	else if (objectClass == Class_Plane)
		return intersect::rayPlane(ray, plane(equation), &point);
	
	return false;
}

et::vec3 SceneObject::normalFromPoint(const vec3& pt) const
{
	if (objectClass == Class_Plane)
		return equation.xyz();
	
	if ((objectClass == Class_Sphere) || (objectClass == Class_SphericalLight))
		return pt - equation.xyz();
		
	return vec3(0.0f);
}

RaytraceScene::RaytraceScene()
{
	vec3 boxSize(60.0f, 25.0f, 35.0f);
	float r = 8.0f;

	camera.perspectiveProjection(QUARTER_PI, 1.0f, 1.0f, 1024.0f);
	camera.lookAt(vec3(0.0f, 0.0f, 2.0f * boxSize.z));
		
	lightSphere = vec4(-boxSize.x, boxSize.y + 0.1f * r, 0.75f * boxSize.z, 2.0f * r);
	lightColor = vec4(30.0f, 35.0f, 45.0f, 1.0f);
	objects.push_back(SceneObject(SceneObject::Class_SphericalLight, lightSphere, vec4(0.0f), lightColor));

	lightSphere = vec4(boxSize.x + r, boxSize.y + 0.1f * r, 0.75f * boxSize.z, 2.0f * r);
	lightColor = vec4(45.0f, 35.0f, 30.0f, 1.0f);
	objects.push_back(SceneObject(SceneObject::Class_SphericalLight, lightSphere, vec4(0.0f), lightColor));
	
	float a = 0.0f;
	float da = DOUBLE_PI / 3.0f;
	spheres.push_back(vec4(4.0f * r * std::cos(a), -boxSize.y + r, 2.0f * r * std::sin(a), r)); a += da;
	sphereColors.push_back(vec4(1.0f, 0.5f, 0.25f, 0.2f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, spheres.back(), sphereColors.back(), vec4(0.0f)));

	spheres.push_back(vec4(2.0f * r * std::cos(a), -boxSize.y + 1.5f * r, 2.0f * r * std::sin(a), 1.5f * r)); a += da;
	sphereColors.push_back(vec4(0.25f, 1.0f, 0.5f, 0.015f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, spheres.back(), sphereColors.back(), vec4(0.0f)));
	
	spheres.push_back(vec4(4.0f * r * std::cos(a), -boxSize.y + 0.75f * r, 2.0f * r * std::sin(a), 0.75f * r)); a += da;
	sphereColors.push_back(vec4(0.5f, 0.25f, 1.0f, 0.75f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, spheres.back(), sphereColors.back(), vec4(0.0f)));

	// left
	planes.push_back(vec4(1.0f, 0.0f, 0.0f, -boxSize.x));
	planeColors.push_back(vec4(0.25f, 0.5f, 1.0f, 0.999915f));
	objects.push_back(SceneObject(SceneObject::Class_Plane, planes.back(), planeColors.back(), vec4(0.0f)));
	
	// right
	planes.push_back(vec4(-1.0f, 0.0f, 0.0f, -boxSize.x));
	planeColors.push_back(vec4(1.0f, 0.5f, 0.25f, 0.9999915f));
	objects.push_back(SceneObject(SceneObject::Class_Plane, planes.back(), planeColors.back(), vec4(0.0f)));
	
	// top
	planes.push_back(vec4(0.0f, -1.0f, 0.0f, -boxSize.y));
	planeColors.push_back(vec4(1.0f, 0.9999915f));
	objects.push_back(SceneObject(SceneObject::Class_Plane, planes.back(), planeColors.back(), vec4(0.0f)));
	
	// bottom
	planes.push_back(vec4(0.0f, 1.0f, 0.0f, -boxSize.y));
	planeColors.push_back(vec4(1.0f/3.0f, 0.5f));
	objects.push_back(SceneObject(SceneObject::Class_Plane, planes.back(), planeColors.back(), vec4(0.0f)));
	
	// back
	planes.push_back(vec4(0.0f, 0.0f, 1.0f, -boxSize.z));
	planeColors.push_back(vec4(0.75f, 0.999915f));
	objects.push_back(SceneObject(SceneObject::Class_Plane, planes.back(), planeColors.back(), vec4(0.0f)));
	
	// front
	planes.push_back(vec4(0.0f, 0.0f, -1.0f, -boxSize.z));
	planeColors.push_back(vec4(0.75f, 1.0f, 1.0f, 1.0f));
	objects.push_back(SceneObject(SceneObject::Class_Plane, planes.back(), planeColors.back(), vec4(0.0f)));
}

const SceneObject& RaytraceScene::objectAtIndex(int i) const
{
	if ((i == missingObject) || (i >= objects.size()))
		return emptyObject;
		
	return objects.at(i);
}
