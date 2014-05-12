/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once


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
		AABBCorner_max
	};

	static const AABBCorner AABBCorner_First = AABBCorner_LeftUpFar;

	struct AABB
	{
		typedef vec4 AABBCorners[AABBCorner_max];

		vec3 center;
		vec3 dimension;
		AABBCorners corners; 

		AABB()
			{ }
		
		AABB(const vec3& aCenter, const vec3& aDimension) :
			center(aCenter), dimension(aDimension)
		{
			corners[AABBCorner_LeftDownFar] = vec4(center + vec3(-dimension.x, -dimension.y, -dimension.z), 1.0f);
			corners[AABBCorner_RightDownFar] = vec4(center + vec3(+dimension.x, -dimension.y, -dimension.z), 1.0f);
			corners[AABBCorner_LeftUpFar] = vec4(center + vec3(-dimension.x, +dimension.y, -dimension.z), 1.0f);
			corners[AABBCorner_RightUpFar] = vec4(center + vec3(+dimension.x, +dimension.y, -dimension.z), 1.0f);
			corners[AABBCorner_LeftDownNear] = vec4(center + vec3(-dimension.x, -dimension.y, +dimension.z), 1.0f);
			corners[AABBCorner_RightDownNear] = vec4(center + vec3(+dimension.x, -dimension.y, +dimension.z), 1.0f);
			corners[AABBCorner_LeftUpNear] = vec4(center + vec3(-dimension.x, +dimension.y, +dimension.z), 1.0f);
			corners[AABBCorner_RightUpNear] = vec4(center + vec3(+dimension.x, +dimension.y, +dimension.z), 1.0f);
		}

		vec3 minVertex() const
			{ return center - dimension; }

		vec3 maxVertex() const
			{ return center + dimension; }
	};

}
