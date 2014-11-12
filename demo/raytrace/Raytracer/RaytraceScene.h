//
//  RaytraceScene.h
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et/camera/camera.h>
#include <et/scene3d/scene3d.h>

namespace rt
{
	struct Intersection
	{
	public:
		enum : int
		{
			missingObject = -1
		};
		
		typedef std::vector<Intersection, et::SharedBlockAllocatorSTDProxy<Intersection>> List;
		
	public:
		int objectIndex = missingObject;
		
		et::ray3d outgoingRay;
		
		et::vec3 hitPoint;
		et::vec3 hitNormal;
		
		Intersection() { }
		
		Intersection(int o) :
			objectIndex(o) { }
	};
	
	struct SceneMaterial
	{
		et::vec4 diffuseColor = et::vec4(0.5f);
		et::vec4 reflectiveColor = et::vec4(1.0f);
		et::vec4 emissiveColor = et::vec4(0.5f);
		
		float refractiveIndex = 0.0f;
		float roughness = 1.0f;
		
		SceneMaterial()
			{ }
		
		SceneMaterial(const et::vec4& kD, const et::vec4& kR, const et::vec4& kE, float rg, float ior = 0.0f) :
			diffuseColor(kD), reflectiveColor(kR), emissiveColor(kE), roughness(rg), refractiveIndex(ior) { }
	};
	
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
		size_t materialId = static_cast<size_t>(Intersection::missingObject);
		
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
	
	struct RaytraceScene
	{
	public:
		void load(et::RenderContext*);
		
		const SceneObject& objectAtIndex(int) const;
		const SceneMaterial& materialAtIndex(size_t) const;
		
	public:
		et::RenderContext* _rc;
		
		et::Camera camera;
		
		float apertureSize = 1.0f / 2.8f;
		uint32_t apertureBlades = 5;
		
		et::vec4 ambientColor = et::vec4(0.0f);
		et::TextureDescription::Pointer environmentMap;
		
		std::vector<SceneObject, et::SharedBlockAllocatorSTDProxy<SceneObject>> objects;
		std::vector<SceneMaterial, et::SharedBlockAllocatorSTDProxy<SceneMaterial>> materials;
		
		struct
		{
			int samples = 16;
			int bounces = 10;
			float exposure = 1.5f;
			
		} options;
		
		SceneObject emptyObject;
		SceneMaterial defaultMaterial;
	};
}
