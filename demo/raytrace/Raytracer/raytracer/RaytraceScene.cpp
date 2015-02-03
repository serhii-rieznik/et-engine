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

using namespace et;
using namespace rt;

void RaytraceScene::load(et::RenderContext* rc)
{
	apertureSize = 0.0f;
	ambientColor = vec4(2.0f);
	
	environmentMap = loadTexture(application().resolveFileName("sun-fixed.hdr"));
	
	camera.perspectiveProjection(QUARTER_PI, 1.0f, 1.0f, 1024.0f);
		
	ObjectsCache cache;
	OBJLoader loader(rc, "shaderart.obj");
	auto loadedModel = loader.load(cache, OBJLoader::Option_SupportMeshes);
	
	s3d::Scene scene;
	scene.deserialize(application().resolveFileName("machine/vending_machine_07.etm"), rc, cache);

	auto meshes = scene.childrenOfType(s3d::ElementType_Mesh);
	for (s3d::Mesh::Pointer m : meshes)
	{
		auto mat = m->material();
		
		vec4 kD = mat->getVector(MaterialParameter_DiffuseColor);
		vec4 kS = mat->getVector(MaterialParameter_SpecularColor);
		vec4 kE = mat->getVector(MaterialParameter_EmissiveColor);
		float Ns = etMin(1.0f, mat->getFloat(MaterialParameter_Roughness));
		float Tr = mat->getFloat(MaterialParameter_Transparency);
		materials.emplace_back(kD, kS, kE, Ns, Tr);
		
		size_t materialIndex = materials.size() - 1;
		size_t firstTriangleIndex = _triangles.lastElementIndex();
		size_t numTriangles = m->numIndexes() / 3;
		
		log::info("Mesh: %s, triangles: %llu, material: %s :", m->name().c_str(), (uint64_t)numTriangles, mat->name().c_str());
		log::info("{");
		log::info("\tdiffuse = %.3f, %.3f, %.3f", kD.x, kD.y, kD.z);
		log::info("\tspecular = %.3f, %.3f, %.3f", kS.x, kS.y, kS.z);
		log::info("\temissive = %.3f, %.3f, %.3f", kE.x, kE.y, kE.z);
		log::info("\troughness = %.3f", Ns);
		log::info("\ttransparency = %.3f", Tr);
		log::info("}");
		
		_triangles.fitToSize(numTriangles);
		
		const auto& vs = m->vertexStorage();
		if (vs.valid() && (numTriangles > 0))
		{
			const auto pos = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Position, 0);
			const auto nrm = vs->accessData<VertexAttributeType::Vec3>(VertexAttributeUsage::Normal, 0);
			
			size_t i0 = m->startIndex();
			const auto& ia = m->indexArray();
			for (uint32_t i = 0; i < numTriangles; ++i)
			{
				size_t v1 = ia->getIndex(i0 + 3*i+0);
				size_t v2 = ia->getIndex(i0 + 3*i+1);
				size_t v3 = ia->getIndex(i0 + 3*i+2);
				_triangles.push_back(SceneTriangle(pos[v1], pos[v2], pos[v3], nrm[v1], nrm[v2], nrm[v3], materialIndex));
			}
			objects.push_back(sharedObjectFactory().createObject<MeshObject>(firstTriangleIndex, numTriangles, _triangles));
		}
		else
		{
			log::warning("Unable to get triangles from this mesh.");
		}
	}
}

const SceneMaterial& RaytraceScene::materialAtIndex(size_t i) const
{
	if (i >= materials.size())
		ET_FAIL("Invalid material index");
	
	return materials.at(i);
}

SceneIntersection RaytraceScene::findNearestIntersection(const et::ray3d& inRay) const
{
	SceneIntersection result;
	
	vec3 point;
	vec3 normal;
	size_t materialIndex = 0;
	
	for (const auto& obj : objects)
	{
		if (obj->intersects(inRay, point, normal, materialIndex))
		{
			float hitDistance = (point - inRay.origin).dotSelf();
			if (hitDistance < result.rayDistance)
			{
				result.hitPoint = point;
				result.hitNormal = normal;
				result.rayDistance = hitDistance;
				result.materialIndex = materialIndex;
				result.objectHit = true;
			}
		}
	}
	
	return result;
}
