/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/collision/aabb.h>

using namespace et;

AABB::AABB(const vec3& aCenter, const vec3& aDimension) : center(aCenter), dimension(aDimension)
{
  corners[AABBCorner_LeftDownFar]   = vec4( center + vec3( -dimension.x, -dimension.y, -dimension.z), 1.0f);
  corners[AABBCorner_RightDownFar]  = vec4( center + vec3( +dimension.x, -dimension.y, -dimension.z), 1.0f);
  corners[AABBCorner_LeftUpFar]     = vec4( center + vec3( -dimension.x, +dimension.y, -dimension.z), 1.0f);
  corners[AABBCorner_RightUpFar]    = vec4( center + vec3( +dimension.x, +dimension.y, -dimension.z), 1.0f);
  corners[AABBCorner_LeftDownNear]  = vec4( center + vec3( -dimension.x, -dimension.y, +dimension.z), 1.0f);
  corners[AABBCorner_RightDownNear] = vec4( center + vec3( +dimension.x, -dimension.y, +dimension.z), 1.0f);
  corners[AABBCorner_LeftUpNear]    = vec4( center + vec3( -dimension.x, +dimension.y, +dimension.z), 1.0f);
  corners[AABBCorner_RightUpNear]   = vec4( center + vec3( +dimension.x, +dimension.y, +dimension.z), 1.0f);
}
