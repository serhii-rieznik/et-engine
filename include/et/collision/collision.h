/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/geometry.h>
#include <et/collision/sphere.h>
#include <et/collision/aabb.h>
#include <et/collision/obb.h>

namespace et
{
	namespace intersect
	{
		/*
		 * Sphere intersections
		 */
		bool sphereSphere(const Sphere& s1, const Sphere& s2, vec3* amount);
		bool sphereBox(const vec3& sphereCenter, float sphereRadius, const vec3& boxCenter, const vec3& boxExtent);
		bool sphereAABB(const Sphere& s, const AABB& b);
		bool sphereOBB(const Sphere& s, const OBB& b);
		
		/*
		 * Sphere-triangles intersections
		 */
		bool sphereTriangle(const vec3& sphereCenter, const float sphereRadius, const triangle& t,
			vec3& point, vec3& normal, float& penetration);
		
		bool sphereTriangle(const Sphere& s, const triangle& t, vec3& point, vec3& normal,
			float& penetration);
		
		bool sphereTriangles(const Sphere& s, const triangle* triangles, size_t triangleCount,
			vec3& point, vec3& normal, float& penetration);
		
		bool sphereTriangle(const Sphere& s, const vec3& sphereVelocity, const triangle& t,
			vec3& point, vec3& normal, float& penetration, float& intersectionTime);
		
		bool sphereTriangles(const Sphere& s, const vec3& velocity, const triangle* triangles,
			size_t triangleCount, vec3& point, vec3& normal, float& penetration,
			float& intersectionTime);

		/*
		 * Ray intersections
		 */
		bool raySphere(const ray3d&, const Sphere& s, vec3* ip1, vec3* ip2 = nullptr);
		bool rayPlane(const ray3d&, const plane& p, vec3* intersection_pt);
		bool rayAABB(et::ray3d r, const et::AABB&);
		bool raySegment(const ray2d&, const segment2d&, vec2* intersectionPoint);
		
		/*
		 * Ray-triangles intersections
		 */
		bool rayTriangle(const ray3d&, const triangle& t, vec3* intersection_pt);
		bool rayTriangleTwoSided(const ray3d&, const triangle& t, vec3* intersection_pt);
		bool rayTriangles(const ray3d&, const triangle* triangles, size_t triangleCount, vec3* intersection_pt);

		/*
		 * Segmenet intersections
		 */
		bool segmentPlane(const segment3d& s, const plane& p, vec3* intersection_pt);
		bool segmentTriangle(const segment3d& s, const triangle& t, vec3* intersection_pt);
		
		/*
		 * Other intersections
		 */
		bool aabbAABB(const AABB& a1, const AABB& a2);
		bool triangleTriangle(const triangle& t1, const triangle& t2);
	}

	vec2 barycentricCoordinates(vec3, const triangle& t);
	vec3 worldCoordinatesFromBarycentric(const vec2& b, const triangle& t);
	vec3 closestPointOnTriangle(const vec3& p, const triangle& t);

	bool pointInsideTriangle(const vec3& p, const triangle& t);
	bool pointInsideTriangle(const vec3& p, const triangle& t, const vec3& n);
	
	bool pointInsidePolygon(const vec2& p, const std::vector<vec2>& polygon);

	float distanceSquareFromPointToLine(const vec3& p, const vec3& l0, const vec3& l1, vec3& projection);

	inline float distanceFromPointToLine(const vec3& p, const vec3& l0, const vec3& l1, vec3& projection)
		{ return sqrtf(distanceSquareFromPointToLine(p, l0, l1, projection)); }

}
