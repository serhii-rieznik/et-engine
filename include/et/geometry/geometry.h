/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/ray.h>
#include <et/geometry/triangleex.h>
#include <et/geometry/segment2d.h>
#include <et/geometry/segment3d.h>
#include <et/geometry/plane.h>
#include <et/geometry/sphere.h>
#include <et/geometry/equations.h>
#include <et/geometry/boundingbox.h>

namespace et
{	
	typedef Line2d<float> line2d;
	
	typedef Ray2d<float> ray2d;
	typedef Ray3d<float> ray3d;
	
	typedef Segment2d<float> segment2d;
	typedef Segment3d<float> segment3d;
	
	typedef Triangle<float> triangle;
	typedef TriangleEx<float> triangleEx;
	
	typedef Plane<float> plane;
	
	template<typename T>
	inline T sqr(T value) 
		{ return value*value; }

	inline float length(float v)
		{ return std::abs(v); }
	
	inline float mix(float v1, float v2, float t)
		{ return v1 * (1.0f - t) + v2 * t; }
	
	template <typename T>
	Triangle<T> operator * (const matrix3<T>& m, const Triangle<T>& t)
		{ return Triangle<T>(m * t.v1, m * t.v2, m * t.v3); }

	template <typename T>
	Triangle<T> operator * (const matrix4<T>& m, const Triangle<T>& t)
		{ return Triangle<T>(m * t.v1(), m * t.v2(), m * t.v3()); }
	
	template <typename T>
	inline matrix4<T> orientationForNormal(const vector3<T>& n)
	{
		vector3<T> up = normalize(n);
		T theta = asin(up.y) - static_cast<T>(HALF_PI);
		T phi = atan2(up.z, up.x) + static_cast<T>(HALF_PI);
		T csTheta = cos(theta);
		vector3<T> side2(csTheta * cos(phi), sin(theta), csTheta * sin(phi));
		vector3<T> side1 = up.cross(side2);
		
		matrix4<T> result(T(1));
		result[0].xyz() = vector3<T>(side1.x, up.x, side2.x);
		result[1].xyz() = vector3<T>(side1.y, up.y, side2.y);
		result[2].xyz() = vector3<T>(side1.z, up.z, side2.z);
		return result;
	}	

	template <typename T>
	inline vector2<T> multiplyWithoutTranslation(const vector2<T>& v, const matrix4<T>& m)
		{ return vector2<T>(m[0][0] * v.x + m[1][0] * v.y, m[0][1] * v.x + m[1][1] * v.y); }

	template <typename T>
	inline vector2<T> operator * (const matrix4<T>& m, const vector2<T>& v)
	{
		return vector2<T>(m[0][0] * v.x + m[1][0] * v.y + m[3][0],
			m[0][1] * v.x + m[1][1] * v.y + m[3][1]);
	}

	inline void normalizeAngle(float& angle)
	{
		while (angle < -PI) angle += DOUBLE_PI;
		while (angle > PI) angle -= DOUBLE_PI;
	}
	
	template <typename T>
	inline T intPower(T value, size_t power)
	{
		T result = static_cast<T>(1);
		for (size_t i = 1; i <= power; ++i)
			result *= value;
		return result;
	}
	
	template <typename T>
	T bezierCurve(T* points, size_t size, float time)
	{
		ET_ASSERT((size > 0) && (points != nullptr));

		if (size == 1)
			return *points;
		
		if (size == 2)
			return mix(*points, points[1], time);

		if (size == 3)
		{
			float invTime = 1.0f - time;
			return sqr(invTime) * points[0] + 2.0f * invTime * time * points[1] + sqr(time) * points[2];
		}
		
		return mix(bezierCurve<T>(points, size-1, time), bezierCurve<T>(points+1, size-1, time), time);
	}
	
	quaternion matrixToQuaternion(const mat3& m);
	quaternion matrixToQuaternion(const mat4& m);

	vec3 removeMatrixScale(mat3& m);
	void decomposeMatrix(const mat4& mat, vec3& translation, quaternion& rotation, vec3& scale);
	
	vec3 randVector(float sx = 1.0f, float sy = 1.0f, float sz = 1.0f);
	
	uint32_t randomInteger(uint32_t limit = RAND_MAX);
	
	float randomFloat(float low, float up);
	float randomFloat();
	
	float signOrZero(float s);
	float signNoZero(float s);

	mat4 rotation2DMatrix(float angle);
	mat4 transform2DMatrix(float a, const vec2& scale, const vec2& translate);
	mat3 rotation2DMatrix3(float angle);
	
	vec3 circleFromPoints(const vec2& p1, const vec2& p2, const vec2& p3);
	vec3 perpendicularVector(const vec3&);
	vec3 randomVectorOnHemisphere(const vec3& normal, float distributionAngle);
	vec3 randomVectorOnDisk(const vec3& normal);

	vec3 rotateAroundVector(const vec3& axis, const vec3& v, float);

	quaternion quaternionFromAngles(float x, float y, float z);
}
