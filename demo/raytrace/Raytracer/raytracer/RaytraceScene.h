//
//  RaytraceScene.h
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et/camera/camera.h>

#include "SceneObject.h"
#include "SceneMaterial.h"

namespace rt
{
	struct RaytraceScene
	{
	public:
		void load(et::RenderContext*);
		
		const SceneObject* objectAtIndex(size_t) const;
		const SceneMaterial& materialAtIndex(size_t) const;
		
	public:
		struct
		{
			size_t samples = 16;
			size_t bounces = 10;
			float exposure = 1.5f;
		} options;
		
	public:
		et::vec4 ambientColor = et::vec4(0.0f);
		
		et::RenderContext* _rc = nullptr;
		
		std::vector<SceneObject*, et::SharedBlockAllocatorSTDProxy<SceneObject*>> objects;
		std::vector<SceneMaterial, et::SharedBlockAllocatorSTDProxy<SceneMaterial>> materials;
		
		et::TextureDescription::Pointer environmentMap;
		
		DummyObject _dummyObject;
		SceneMaterial _dummyMaterial;
		
		et::Camera camera;
		float apertureSize = 1.0f / 2.8f;
		uint32_t apertureBlades = 5;
	};
}
