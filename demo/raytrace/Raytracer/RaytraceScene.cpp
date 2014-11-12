//
//  RaytraceScene.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/app/application.h>
#include <et/models/objloader.h>
#include <et/imaging/textureloader.h>
#include "RaytraceScene.h"

using namespace rt;
using namespace et;

SceneObject::SceneObject(const et::s3d::SupportMesh::Pointer& m, size_t mat) :
	objectClass(Class_Mesh), materialId(mat)
{
	mesh = m->triangles();
	equation = vec4(m->center(), m->radius());
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
	apertureSize = 0.0f;
	ambientColor = vec4(5.0f);
	environmentMap = loadTexture(application().resolveFileName("textures/background.hdr"));
	
	camera.perspectiveProjection(QUARTER_PI, 1.0f, 1.0f, 1024.0f);
	
	ObjectsCache cache;
	OBJLoader loader(rc, "models/CornellBox-Original.obj");
	auto loadedModel = loader.load(cache, OBJLoader::Option_SupportMeshes);
	auto meshes = loadedModel->childrenOfType(s3d::ElementType_SupportMesh);
	
	for (s3d::SupportMesh::Pointer m : meshes)
	{
		log::info("Mesh: %s, triangles: %zu", m->name().c_str(), m->triangles().size());
		if (m->triangles().size() > 0)
		{
			objects.emplace_back(m, materials.size());
			
			materials.emplace_back(m->material()->getVector(MaterialParameter_AmbientColor) +
				m->material()->getVector(MaterialParameter_DiffuseColor), vec4(0.0f),
				m->material()->getVector(MaterialParameter_EmissiveColor), 0.5f);
		}
	}
	return;
}

const SceneObject& RaytraceScene::objectAtIndex(int i) const
{
	if ((i == Intersection::missingObject) || (i >= static_cast<int>(objects.size())))
		return emptyObject;
		
	return objects.at(i);
}

const SceneMaterial& RaytraceScene::materialAtIndex(size_t i) const
{
	if (i >= materials.size())
		return defaultMaterial;
	
	return materials.at(i);
}