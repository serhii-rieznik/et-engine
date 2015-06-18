/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/vector3.h>

namespace et
{
	enum AABBCorner
	{ 
		AABBCorner_LeftUpFar,
		AABBCorner_RightUpFar,
		AABBCorner_LeftDownFar,
		AABBCorner_RightDownFar,
		AABBCorner_LeftUpNear,
		AABBCorner_RightUpNear,
		AABBCorner_LeftDownNear,
		AABBCorner_RightDownNear,
		AABBCorner_max,
		
		AABBCorner_First = AABBCorner_LeftUpFar
	};
	
	struct AABB
	{
		typedef vector3<float> AABBCorners[AABBCorner_max];

		vector3<float> center = vector3<float>(0.0f);
		vector3<float> halfDimension = vector3<float>(0.0f);
		
		AABBCorners corners; 

		AABB() { }
		
		AABB(const vector3<float>& aCenter, const vector3<float>& aHalfDimension) :
			center(aCenter), halfDimension(aHalfDimension)
		{
			corners[AABBCorner_LeftDownFar] = center + vector3<float>(-halfDimension.x, -halfDimension.y, -halfDimension.z);
			corners[AABBCorner_RightDownFar] = center + vector3<float>(halfDimension.x, -halfDimension.y, -halfDimension.z);
			corners[AABBCorner_LeftUpFar] = center + vector3<float>(-halfDimension.x, halfDimension.y, -halfDimension.z);
			corners[AABBCorner_RightUpFar] = center + vector3<float>(halfDimension.x, halfDimension.y, -halfDimension.z);
			corners[AABBCorner_LeftDownNear] = center + vector3<float>(-halfDimension.x, -halfDimension.y, halfDimension.z);
			corners[AABBCorner_RightDownNear] = center + vector3<float>(halfDimension.x, -halfDimension.y, halfDimension.z);
			corners[AABBCorner_LeftUpNear] = center + vector3<float>(-halfDimension.x, halfDimension.y, halfDimension.z);
			corners[AABBCorner_RightUpNear] = center + vector3<float>(halfDimension.x, halfDimension.y, halfDimension.z);
		}

		const vector3<float>& minVertex() const
			{ return corners[AABBCorner_LeftDownFar]; }

		const vector3<float>& maxVertex() const
			{ return corners[AABBCorner_RightUpNear]; }
	};

}
