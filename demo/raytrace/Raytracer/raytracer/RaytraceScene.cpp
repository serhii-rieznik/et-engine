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
	apertureSize = 0.125f;
	
	ambientColor = vec4(3.0f);
	
	environmentMap = loadTexture(application().resolveFileName("background.hdr"));
	
	camera.perspectiveProjection(QUARTER_PI, 1.0f, 1.0f, 1024.0f);
		
	ObjectsCache cache;
	OBJLoader loader(rc, "sphereincube.obj");
	auto loadedModel = loader.load(cache, OBJLoader::Option_SupportMeshes);
	auto meshes = loadedModel->childrenOfType(s3d::ElementType_SupportMesh);
	
	for (s3d::SupportMesh::Pointer m : meshes)
	{
		if (m->triangles().size() > 0)
		{
			auto mat = m->material();
			vec4 kD = mat->getVector(MaterialParameter_DiffuseColor);
			vec4 kS = mat->getVector(MaterialParameter_SpecularColor);
			vec4 kE = mat->getVector(MaterialParameter_EmissiveColor);
			float Ns = etMin(1.0f, mat->getFloat(MaterialParameter_Roughness));
			float Tr = mat->getFloat(MaterialParameter_Transparency);
			
			log::info("Mesh: %s, triangles: %llu, material: %s :", m->name().c_str(), (uint64_t)m->triangles().size(), mat->name().c_str());
			log::info("{");
			log::info("\tdiffuse = %.3f, %.3f, %.3f", kD.x, kD.y, kD.z);
			log::info("\tspecular = %.3f, %.3f, %.3f", kS.x, kS.y, kS.z);
			log::info("\temissive = %.3f, %.3f, %.3f", kE.x, kE.y, kE.z);
			log::info("\troughness = %.3f", Ns);
			log::info("\ttransparency = %.3f", Tr);
			log::info("}");
			
			materials.emplace_back(kD, kS, kE, Ns, Tr);
			objects.emplace_back(sharedObjectFactory().createObject<MeshObject>(m, materials.size() - 1));
		}
	}
}

const SceneObject* RaytraceScene::objectAtIndex(size_t i) const
{
	return i < objects.size() ? objects.at(i) : &_dummyObject;
}

const SceneMaterial& RaytraceScene::materialAtIndex(size_t i) const
{
	return (i < materials.size()) ? materials.at(i) : _dummyMaterial;
}
