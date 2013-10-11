/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/geometry/matrix4.h>

namespace et
{
	template <typename T>
	class Triangle
	{
	public:
		Triangle() { }

		Triangle(const vector3<T>& av1, const vector3<T>& av2, const vector3<T>& av3) : 
			_v1(av1), _v2(av2), _v3(av3) { fillSupportData(); }

		Triangle(vector3<T>&& av1, vector3<T>&& av2, vector3<T>&& av3) : 
			_v1(av1), _v2(av2), _v3(av3) { fillSupportData(); }

		const vector3<T>& v1() const 
			{ return _v1; }

		const vector3<T>& v2() const
			{ return _v2; }

		const vector3<T>& v3() const
			{ return _v3; }

		const vector3<T>& edge2to1() const 
			{ return _edge2to1; }

		const vector3<T>& edge3to1() const
			{ return _edge3to1; }

		const vector3<T>& edge3to2() const
			{ return _edge3to2; }

		const vector3<T>& normalizedNormal() const
			{ return _normal; }

		T square()
		{
			T a = _edge2to1.length();
			T b = _edge3to1.length();
			T c = _edge3to2.length();
			T p = (a + b + c) / static_cast<T>(2);
			return std::sqrt(p * (p - a) * (p - b) * (p - c));
		}

	private:
		
		void fillSupportData()
		{
			_edge2to1 = _v2 - _v1;
			_edge3to1 = _v3 - _v1;
			_edge3to2 = _v3 - _v2;
			_normal = normalize(cross(_edge2to1, _edge3to1));
		}

	private:
		vector3<T> _v1;
		vector3<T> _v2;
		vector3<T> _v3;
		vector3<T> _normal;
		vector3<T> _edge2to1;
		vector3<T> _edge3to1;
		vector3<T> _edge3to2;
	};

}