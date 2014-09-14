/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/geometry/vector3.h>

namespace et
{
	template <typename T>
	struct Line2d
	{
	public:
		/**
		 * A, B and C coefficients in equation Ax + By + C = 0
		 */
		vector3<T> equation;
		
	public:
		Line2d()
			{ }
		
		Line2d(T a, T b, T c) :
			equation(a, b, c) { }
		
		Line2d(const vector2<T>& p1, const vector2<T>& p2) :
			equation(p1.y - p2.y, p2.x - p1.x, p1.x * p2.y - p2.x * p1.y) { }

	public:
		/**
		 * Distance from point to line.
		 */
		T pointDistance(const vector2<T>& point);
		
		/**
		 * Performs checking if line intersects another line, writes point of intersection into the
		 * point variable.
		 */
		bool intersects(const Line2d<T>& line, vector2<T>* point);
		
		/**
		 * Returns K and B in the equation y = Kx + B.
		 */
		vector2<T> slopeInterceptForm();
	};

	template <typename T>
	T Line2d<T>::pointDistance(const vector2<T>& point)
	{
		return (equation.x * point.x + equation.y * point.y + equation.z) /
			std::sqrt(equation.x*equation.x + equation.y * equation.y);
	}
	
	template <typename T>
	bool Line2d<T>::intersects(const Line2d<T>& l, vector2<T>* point)
	{
		T det = equation.x * l.equation.y - equation.y * l.equation.x;
		if (std::abs(det) < std::numeric_limits<T>::epsilon()) return false;
		
		if (point != nullptr)
		{
			*point = vector2<T>(equation.y * l.equation.z - equation.z * l.equation.y,
				equation.z * l.equation.x - equation.x * l.equation.z) / det;
		}
		return true;
	}
	
	template <typename T>
	vector2<T> Line2d<T>::slopeInterceptForm()
	{
		ET_ASSERT(std::abs(equation.y) > std::numeric_limits<T>::epsilon());
		return vector2<T>(-equation.x / equation.y, -equation.z / equation.y);
	}
}
