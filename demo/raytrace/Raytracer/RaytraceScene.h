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
	struct SceneObject
	{
		enum Class
		{
			Class_None,
			Class_Sphere,
			Class_SphericalLight,
			Class_Plane,
		};
		
		Class objectClass = Class_None;
		
		et::vec4 equation;
		et::vec4 color;
		et::vec4 emissive;
		float roughness = 1.0f;
		
		SceneObject()
			{ }
		
		SceneObject(Class cls, const et::vec4& eq, const et::vec4& c, const et::vec4& em) :
			objectClass(cls), equation(eq), color(c), emissive(em)
		{
			roughness = color.w;
		}
		
		bool intersectsRay(const et::ray3d&, et::vec3& point) const;
		et::vec3 normalFromPoint(const et::vec3&) const;
		
		size_t objectId = 0;
	};
	
	struct RaytraceScene
	{
	public:
		enum : int
		{
			missingObject = -1
		};
		
		RaytraceScene();
		
		const SceneObject& objectAtIndex(int) const;
		
	public:
		et::Camera camera;
		
		et::vec4 lightSphere;
		et::vec4 lightColor;
		
		std::vector<SceneObject> objects;
		
		std::vector<et::vec4> planes;
		std::vector<et::vec4> planeColors;
		std::vector<et::vec4> spheres;
		std::vector<et::vec4> sphereColors;
		
		struct
		{
			int samples = 16;
			int bounces = 10;
			float exposure = 1.0f;
			
		} options;
		
		SceneObject emptyObject;
	};
}
