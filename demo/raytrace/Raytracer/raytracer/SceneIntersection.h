//
//  SceneIntersection.h
//  Raytracer
//
//  Created by Sergey Reznik on 16/11/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et/collision/collision.h>
#include <et/scene3d/scene3d.h>

namespace rt
{
	struct SceneIntersection
	{
	public:
		SceneIntersection()
			{ }
		
		SceneIntersection(size_t m) :
			materialIndex(m) { }
		
	public:
		et::vec3 hitPoint;
		et::vec3 hitNormal;
		
		size_t materialIndex = 0;
		
		float rayDistance = std::numeric_limits<float>::max();
		
		bool objectHit = false;
	};
	
	bool rayAABB(et::ray3d, const et::AABB&);
}