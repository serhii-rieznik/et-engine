/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/geometry/vector2.h>

namespace et 
{
	template <typename T>
	struct vector3
	{
		union
		{
			struct { T x, y, z; };
			T c[3];
		};

		vector3() :
			x(static_cast<T>(0)), y(static_cast<T>(0)), z(static_cast<T>(0)) { }
		
		vector3(const vector3& m) :
			x(m.x), y(m.y), z(m.z) { }
		
		vector3(vector3&& m) :
			x(m.x), y(m.y), z(m.z) { }
		
		explicit vector3(T val) :
			x(val), y(val), z(val) { }
		
		vector3(T i_x, T i_y) :
			x(i_x), y(i_y), z(static_cast<T>(0)) { }
		
		vector3(T i_x, T i_y, T i_z) :
			x(i_x), y(i_y), z(i_z) { }
		
		vector3(const vector2<T>& i_xy, T i_z) :
			x(i_xy.x), y(i_xy.y), z(i_z) { }

		T* data() 
			{ return c; }

		const T* data() const
			{ return c; }

		char* binary() 
			{ return reinterpret_cast<char*>(c); }

		const char* binary() const
			{ return reinterpret_cast<const char*>(c); }

		T& operator [] (size_t i)
			{ return c[i]; }

		const T& operator [] (size_t i) const
			{ return c[i]; }

		T& operator [] (int i)
			{ return c[i]; }

		const T& operator [] (int i) const
			{ return c[i]; }

		vector3 operator -() const
			{ return vector3(-x, -y, -z); }

		vector3 operator + (const vector3& value) const
			{ return vector3(x + value.x, y + value.y, z + value.z); }

		vector3 operator - (const vector3& value) const
			{ return vector3(x - value.x, y - value.y, z - value.z); }

		vector3 operator * (const vector3& value) const
			{ return vector3(x * value.x, y * value.y, z * value.z); }

		vector3 operator / (const vector3& value) const
			{ return vector3(x / value.x, y / value.y, z / value.z); }

		vector3 operator * (const T& v) const
			{ return vector3(x * v, y * v, z * v); }

		vector3 operator / (const T& v) const
			{ return vector3(x / v, y / v, z / v); }

		vector3& operator += (const vector3 &value)
			{ x+=value.x; y+=value.y; z+=value.z; return *this; }

		vector3& operator -= (const vector3 &value)
			{ x-=value.x; y-=value.y; z-=value.z; return *this; }

		vector3& operator *= (const vector3 &value)
			{ x*=value.x; y*=value.y; z*=value.z; return *this; }

		vector3& operator /= (const vector3 &value)
			{ x/=value.x; y /=value.y; z/=value.z; return *this; }

		vector3& operator *= (T value)
			{ x*=value; y*=value; z*=value; return *this; }

		vector3& operator /= (T value)
			{ x/=value; y /= value; z /= value; return *this; }

		vector3& operator = (const vector3& value)
			{ x = value.x; y = value.y; z = value.z; return *this; }
		
		vector3& operator = (vector3&& value)
			{ x = value.x; y = value.y; z = value.z; return *this; }
		
		T dotSelf() const 
			{ return x*x + y*y + z*z; }

		T length() const 
			{ return std::sqrt( dotSelf() ); }

		vector3 cross(const vector3 &vec) const
			{ return vector3(y * vec.z - z * vec.y, z * vec.x - x * vec.z, 	x * vec.y - y * vec.x ); 	}

		vector2<T>& xy()
			{ return *((vector2<T>*)(c)); }

		const vector2<T>& xy() const
			{ return *((vector2<T>*)(c)); }
		
		void normalize()
		{
			T lenSquare = dotSelf();
			if (lenSquare > 0)
			{
				lenSquare = std::sqrt(lenSquare);
				x /= lenSquare;
				y /= lenSquare;
				z /= lenSquare;
			}
		}
		
		vector3<T> normalized() const
		{
			T lenSquare = dotSelf();
			if (lenSquare > 0)
			{
				lenSquare = std::sqrt(lenSquare);
				return vector3<T>(x / lenSquare, y / lenSquare, z / lenSquare);
			}
			else
			{
				return *this;
			}
		}

		bool operator == (const vector3<T>& r) const
			{ return (x == r.x) && (y == r.y) && (z == r.z); }
		
		bool operator != (const vector3<T>& r) const
			{ return (x != r.x) || (y != r.y) || (z != r.z); }
	};

	template <typename T>
	vector3<T> operator * (T value, const vector3<T>& vec) 
		{ return vector3<T>(vec.x * value, vec.y * value, vec.z * value); }

}
