/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
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
		AABBCorner_max,
		
		AABBCorner_First = AABBCorner_LeftUpFar
	};
	
	struct AABB
	{
		typedef vec3 AABBCorners[AABBCorner_max];

		vec3 center;
		vec3 dimension;
		AABBCorners corners; 

		AABB()
			{ }
		
		AABB(const vec3& aCenter, const vec3& aDimension) :
			center(aCenter), dimension(aDimension)
		{
			corners[AABBCorner_LeftDownFar]   = center + vec3(-dimension.x, -dimension.y, -dimension.z);
			corners[AABBCorner_RightDownFar]  = center + vec3( dimension.x, -dimension.y, -dimension.z);
			corners[AABBCorner_LeftUpFar]     = center + vec3(-dimension.x,  dimension.y, -dimension.z);
			corners[AABBCorner_RightUpFar]    = center + vec3( dimension.x,  dimension.y, -dimension.z);
			corners[AABBCorner_LeftDownNear]  = center + vec3(-dimension.x, -dimension.y,  dimension.z);
			corners[AABBCorner_RightDownNear] = center + vec3( dimension.x, -dimension.y,  dimension.z);
			corners[AABBCorner_LeftUpNear]    = center + vec3(-dimension.x,  dimension.y,  dimension.z);
			corners[AABBCorner_RightUpNear]   = center + vec3( dimension.x,  dimension.y,  dimension.z);
		}

		const vec3& minVertex() const
			{ return corners[AABBCorner_LeftDownFar]; }

		const vec3& maxVertex() const
			{ return corners[AABBCorner_RightUpNear]; }
	};

}
