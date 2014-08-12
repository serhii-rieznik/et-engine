//
//  RaytraceScene.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

//#include <et/models/objloader.h>

#include <et/app/application.h>
#include <et/imaging/textureloader.h>
#include "RaytraceScene.h"

using namespace rt;
using namespace et;

SceneObject::SceneObject(const et::s3d::SupportMesh::Pointer& m, int mat) :
	objectClass(Class_Mesh), materialId(mat)
{
	mesh.resize(m->triangles().size());
	
	vec3 minVertex = mesh[0].v1();
	vec3 maxVertex = minVertex;
	
	size_t i = 0;
	for (const auto& t : m->triangles())
	{
		mesh[i++] = triangle(t.v1(), t.v2(), t.v1());
		
		minVertex = minv(t.v1(), minVertex);
		minVertex = minv(t.v2(), minVertex);
		minVertex = minv(t.v3(), minVertex);
		
		maxVertex = maxv(t.v1(), maxVertex);
		maxVertex = maxv(t.v2(), maxVertex);
		maxVertex = maxv(t.v3(), maxVertex);
	}
	
	equation.xyz() = 0.5f * (minVertex + maxVertex);
	equation.w = (minVertex - maxVertex).length();
	
	log::info("%s loaded", m->name().c_str());
}

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
		if (intersect::raySphere(ray, Sphere(equation.xyz(), equation.w), nullptr))
		{
			for (const auto& t : mesh)
			{
				if (intersect::rayTriangle(ray, t, &point))
				{
					normal = t.normalizedNormal();
					return true;
				}
			}
		}
	}
	
	return false;
}

void RaytraceScene::load(et::RenderContext* rc)
{
	apertureSize = 2.8f;
	ambientColor = vec4(6.0f);
	environmentMap = loadTexture(application().resolveFileName("textures/background.hdr"));
	
	/* *
	ObjectsCache cache;
	OBJLoader loader(rc, "models/test.obj");
	auto loadedModel = loader.load(cache, OBJLoader::Option_SupportMeshes);// | OBJLoader::Option_SwapYwithZ);
	auto meshes = loadedModel->childrenOfType(s3d::ElementType_SupportMesh);
	
	materials.push_back(SceneMaterial(vec4(1.0f, 0.5f, 0.5f, 1.0f), vec4(0.5f, 0.5f, 1.0f, 1.0f), vec4(0.0f), 1.0f, 0.0f));
	for (s3d::SupportMesh::Pointer m : meshes)
	{
		log::info("Mesh: %s, triangles: %zu", m->name().c_str(), m->triangles().size());
		
		if (m->triangles().size() > 0)
			objects.push_back(SceneObject(m, 0));
	}
	return;
	// */
	
	vec3 boxSize(50.0f, 35.0f, 60.0f);
	
	float r0 = 25.0f;
	float r1 = 10.0f;
	float r2 = 15.0f;
	
//	materials.push_back(SceneMaterial(vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(0.0f), 0.0f, 0.0f));
//	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(0.0f, -boxSize.y + r0, 0.0f, r0), 0));
	
	materials.push_back(SceneMaterial(vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(0.0f), 0.00f, 2.148417f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(-boxSize.x + r1, -boxSize.y + r1, -boxSize.z + 2.0f * r1, r1), 0));
	
	materials.push_back(SceneMaterial(vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(0.0f), 0.00f, 0.0f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(-boxSize.x + 1.75f * r2, -boxSize.y + r2,	0.0f, r2), 1));
	
	materials.push_back(SceneMaterial(vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(0.0f), 0.00f, 1.78417f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4(-boxSize.x + r1, -boxSize.y + r1,  boxSize.z - 2.0f * r1, r1), 2));

	materials.push_back(SceneMaterial(vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(1.0f, 1.00f, 1.0f, 1.0f), vec4(0.0f), 0.001f, 1.41f));
	objects.push_back(SceneObject(SceneObject::Class_Sphere, vec4( boxSize.x - 1.25f * r0, -boxSize.y + r0, 0.0f, r0), 3));
	
	int lastMaterial = 0;
	
	/*/ top
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(1.0f), vec4(0.0f), 1.0f));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-boxSize.x, boxSize.y,  boxSize.z),
		vec3(-boxSize.x, boxSize.y, -boxSize.z),
		vec3( boxSize.x, boxSize.y, -boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( boxSize.x, boxSize.y,  boxSize.z),
		vec3(-boxSize.x, boxSize.y,  boxSize.z),
		vec3( boxSize.x, boxSize.y, -boxSize.z), lastMaterial));
	// */
	
	/*/ top light
	float s = 1.0f / 4.0f;
	int lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(1.0f), vec4(50.0f), 1.0f));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3(-s * boxSize.x, 0.99f * boxSize.y,  s * boxSize.z),
		vec3(-s * boxSize.x, 0.99f * boxSize.y, -s * boxSize.z),
		vec3( s * boxSize.x, 0.99f * boxSize.y, -s * boxSize.z), lastMaterial));
	objects.push_back(SceneObject(SceneObject::Class_Triangle,
		vec3( s * boxSize.x, 0.99f * boxSize.y,  s * boxSize.z),
		vec3(-s * boxSize.x, 0.99f * boxSize.y,  s * boxSize.z),
		vec3( s * boxSize.x, 0.99f * boxSize.y, -s * boxSize.z), lastMaterial));
	// */
	
	// bottom
	float bs = 10000.0f;
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(2.0f/3.0f), vec4(2.0f/3.0f), vec4(0.0f), 0.05f, 1.381f));
	objects.push_back(SceneObject(
		vec3(-bs * boxSize.x, -boxSize.y,  bs * boxSize.z),
		vec3( bs * boxSize.x, -boxSize.y, -bs * boxSize.z),
		vec3(-bs * boxSize.x, -boxSize.y, -bs * boxSize.z), lastMaterial));
	objects.push_back(SceneObject(
		vec3(-bs * boxSize.x, -boxSize.y,  bs * boxSize.z),
		vec3( bs * boxSize.x, -boxSize.y,  bs * boxSize.z),
		vec3( bs * boxSize.x, -boxSize.y, -bs * boxSize.z), lastMaterial));

	// left
	float xs = 2.0f;
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f, 1.0f/3.0f, 2.0f/3.0f, 0.0f), vec4(1.0f, 0.5f, 2.0f/3.0f, 0.0f), vec4(0.0f), 0.0f));
	objects.push_back(SceneObject(
		vec3( xs * boxSize.x, -boxSize.y,  boxSize.z),
		vec3( boxSize.x / xs,  boxSize.y, -boxSize.z * xs),
		vec3( boxSize.x / xs, -boxSize.y, -boxSize.z * xs), lastMaterial));
	objects.push_back(SceneObject(
		vec3( xs * boxSize.x, -boxSize.y,  boxSize.z),
		vec3( xs * boxSize.x,  boxSize.y,  boxSize.z),
		vec3( boxSize.x / xs,  boxSize.y, -boxSize.z * xs), lastMaterial));
	// */
	
	// right
	lastMaterial = static_cast<int>(materials.size());
	materials.push_back(SceneMaterial(vec4(1.0f), vec4(1.0f), vec4(0.0f), 0.0f));
	objects.push_back(SceneObject(
		vec3(-xs * boxSize.x, -boxSize.y,  boxSize.z),
		vec3(-boxSize.x / xs, -boxSize.y, -boxSize.z * xs),
		vec3(-boxSize.x / xs,  boxSize.y, -boxSize.z * xs), lastMaterial));
	objects.push_back(SceneObject(
		vec3(-xs * boxSize.x,  boxSize.y,  boxSize.z),
		vec3(-xs * boxSize.x, -boxSize.y,  boxSize.z),
		vec3(-boxSize.x / xs,  boxSize.y, -boxSize.z * xs), lastMaterial));
	 // */
	
	/*/ back
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
	// */
	
	/*/ front
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
	// */

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