/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/geometry/vector4.h>
#include <et/geometry/triangle.h>

namespace et
{
	template <typename T>
	class Plane
	{
	public:
		vector4<T> equation;

		Plane()
			{ }

		explicit Plane(const vector4<T>& eq) :
			equation(eq) { }

		Plane(const vector3<T>& normal, float distance) :
			equation(normal, distance) { }
		
		Plane(float nx, float ny, float nz, float distance) :
			equation(nx, ny, nz, distance) { }

		Plane(const Triangle<T>& t) : equation(t.normalizedNormal(), 0.0)
			{ equation.w = dot(equation.xyz(), t.v1()); }

		const vector3<T>& normal() const 
			{ return equation.xyz(); }

		float distance() const 
			{ return equation.w; }

		vector3<T> planePoint() const
			{ return equation.xyz() * equation.w; }

		vector3<T> projectionOfPoint(const vector3<T>& pt) const
			{ return pt - equation.xyz() * distanceToPoint(pt); }

		float distanceToPoint(const vector3<T>& pt) const
			{ return dot(equation.xyz(), pt) - equation.w; }
		
		vector3<T> reflect(const vector3<T>& point) const
			{ return point - static_cast<T>(2) * (dot(normal(), point) * normal() - distance() * normal()); };
		
	};
}
