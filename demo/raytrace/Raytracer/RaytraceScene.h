//
//  RaytraceScene.h
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et/camera/camera.h>

namespace rt
{
	struct Intersection
	{
	public:
		enum : int
		{
			missingObject = -1
		};
		
		typedef std::vector<Intersection> List;
		
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
		
		SceneMaterial(const et::vec4& d, const et::vec4& refl, const et::vec4& e, float rg, float ior = 0.0f) :
			diffuseColor(d), reflectiveColor(refl), emissiveColor(e), roughness(rg), refractiveIndex(ior) { }
	};
	
	struct SceneObject
	{
		enum Class
		{
			Class_None,
			Class_Sphere,
			Class_Plane,
			Class_Triangle,
		};
		
		Class objectClass = Class_None;
		
		et::triangle tri;
		et::vec4 equation;
		int materialId = Intersection::missingObject;
		
		size_t objectId = 0;

	public:
		SceneObject()
			{ }
		
		SceneObject(Class cls, const et::vec4& eq, int mat) :
			objectClass(cls), equation(eq), materialId(mat) { }

		SceneObject(Class cls, const et::vec3& p1, const et::vec3& p2, const et::vec3& p3, int mat) :
			objectClass(cls), tri(p1, p2, p3), materialId(mat) { }
		
	public:
		bool intersectsRay(const et::ray3d&, et::vec3& point) const;
		
		et::vec3 normalFromPoint(const et::vec3&) const;
	};
	
	struct RaytraceScene
	{
	public:
		void load(et::RenderContext*);
		
		const SceneObject& objectAtIndex(int) const;
		const SceneMaterial& materialAtIndex(int) const;
		
	public:
		et::RenderContext* _rc;
		
		et::Camera camera;
		
		std::vector<SceneObject> objects;
		std::vector<SceneMaterial> materials;
		
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
