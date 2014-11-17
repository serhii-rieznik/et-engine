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
	enum : size_t
	{
		MissingObjectIndex = (size_t)(-1)
	};
	
	struct SceneIntersection
	{
	public:
		SceneIntersection()
			{ }
		
		SceneIntersection(size_t o) :
			hitObjectIndex(o) { }
		
	public:
		et::vec3 hitPoint;
		et::vec3 hitNormal;
		
		size_t hitObjectIndex = MissingObjectIndex;
	};
	
	bool rayAABB(const et::ray3d&, const et::AABB&);
}