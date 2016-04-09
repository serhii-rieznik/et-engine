/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/triangle.h>

namespace et
{
	template <typename T>
	class TriangleEx : public Triangle<T>
	{
	public:
		TriangleEx() { }

		TriangleEx(const vector3<T>& av1, const vector3<T>& av2, const vector3<T>& av3,
				   const vector3<T>& an1, const vector3<T>& an2, const vector3<T>& an3) :
			Triangle<T>(av1, av2, av3), _n1(an1), _n2(an2), _n3(an3) { fillAdditionalSupportData(); }

		const vector3<T>& n1() const
			{ return _n1; }
		
		const vector3<T>& n2() const
			{ return _n2; }
		
		const vector3<T>& n3() const
			{ return _n3; }
		
		vector3<T> barycentric(vector3<T> p) const
		{
			p -= Triangle<T>::v1();
			
			float dot20 = p.dot(Triangle<T>::edge2to1());
			float dot21 = p.dot(Triangle<T>::edge3to1());
			
			vector3<T> result;
			result.y = (_dot11 * dot20 - _dot01 * dot21) * _invDenom;
			result.z = (_dot00 * dot21 - _dot01 * dot20) * _invDenom;
			result.x = T(1) - result.y - result.z;
			return result;
		}
		
		vector3<T> interpolatedPosition(const vector3<T>& bc) const
		{
			return Triangle<T>::v1() * bc.x + Triangle<T>::v2() * bc.y + Triangle<T>::v3() * bc.z;
		}
		
		vector3<T> interpolatedNormal(const vector3<T>& bc) const
		{
			return _n1 * bc.x + _n2 * bc.y + _n3 * bc.z;
		}

	private:
		
		void fillAdditionalSupportData()
		{
			_dot00 = Triangle<T>::edge2to1().dotSelf();
			_dot11 = Triangle<T>::edge3to1().dotSelf();
			_dot01 = Triangle<T>::edge2to1().dot(Triangle<T>::edge3to1());
			_invDenom = T(1) / (_dot00 * _dot11 - _dot01 * _dot01);
		}

	private:
		vector3<T> _n1;
		vector3<T> _n2;
		vector3<T> _n3;
		float _dot00 = 0.0f;
		float _dot01 = 0.0f;
		float _dot11 = 0.0f;
		float _invDenom = 0.0f;
	};

}
