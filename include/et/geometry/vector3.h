/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
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

		vector3()
#	if !defined(ET_DISABLE_VECTOR_INITIALIZATION)
			: x(T(0)), y(T(0)), z(T(0))
#	endif
			{ }
		
		vector3(const vector3& m) :
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

		T& operator [] (uint32_t i)
			{ return c[i]; }

		const T& operator [] (uint32_t i) const
			{ return c[i]; }

		T& operator [] (int32_t i)
			{ return c[i]; }

		const T& operator [] (int32_t i) const
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

		T dotSelf() const
			{ return x*x + y*y + z*z; }

		T length() const 
			{ return std::sqrt( dotSelf() ); }

		vector3 cross(const vector3 &vec) const
			{ return vector3(y * vec.z - z * vec.y, z * vec.x - x * vec.z, 	x * vec.y - y * vec.x ); 	}

		T dot(const vector3& vec) const 
			{ return x * vec.x + y * vec.y + z * vec.z; }

		vector2<T>& xy()
			{ return *((vector2<T>*)(c)); }

		const vector2<T>& xy() const
			{ return *((vector2<T>*)(c)); }
		
		vector3<T>& normalize()
		{
			T lenSquare = dotSelf();
			if (lenSquare > 0)
			{
				lenSquare = std::sqrt(lenSquare);
				x /= lenSquare;
				y /= lenSquare;
				z /= lenSquare;
			}
			return *this;
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

		void setMultiplied(const vector3<T>& m, T a)
		{
			x = a * m.x;
			y = a * m.y;
			z = a * m.z;
		}

		void addMultiplied(const vector3<T>& m, T a)
		{
			x += a * m.x;
			y += a * m.y;
			z += a * m.z;
		}

		void addMultiplied(const vector3<T>& m, const vector3<T>& a)
		{
			x = a.x * m.x + x;
			y = a.y * m.y + y;
			z = a.z * m.z + z;
		}

		void clear()
		{
			x = 0;
			y = 0;
			z = 0;
		}

		bool operator == (const vector3<T>& r) const
			{ return (x == r.x) && (y == r.y) && (z == r.z); }
		
		bool operator != (const vector3<T>& r) const
			{ return (x != r.x) || (y != r.y) || (z != r.z); }
	};

	template <typename T>
	vector3<T> operator * (T value, const vector3<T>& vec) 
		{ return vector3<T>(vec.x * value, vec.y * value, vec.z * value); }

	template<typename T>
	inline vector3<T> absv(const vector3<T>& value)
		{ return vector3<T>(std::abs(value.x), std::abs(value.y), std::abs(value.z)); }
	
	template<typename T>
	inline vector3<T> maxv(const vector3<T>& v1, const vector3<T>& v2)
		{ return vector3<T>(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z)); }
	
	template<typename T>
	inline vector3<T> minv(const vector3<T>& v1, const vector3<T>& v2)
		{ return vector3<T>(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z)); }
		
	template <typename T>
	inline vector3<T> floorv(const vector3<T>& v)
		{ return vector3<T>(std::floor(v.x), std::floor(v.y), std::floor(v.z)); }

	template <typename T>
	inline vector3<T> ceiv(const vector3<T>& v)
		{ return vector3<T>(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z)); }
	
	template <typename T>
	inline vector3<T> sqrtv(const vector3<T>& v)
		{ return vector3<T>(std::sqrt(v.x), std::sqrt(v.y), std::sqrt(v.z)); }
	
	template<typename T>
	inline vector3<T> mix(const vector3<T>& v1, const vector3<T>& v2, const T& t)
		{ return v1 * (static_cast<T>(1) - t) + v2 * t; }
	
	template<typename T>
	inline vector3<T>fromSpherical(T theta, T phi)
	{
		T fCosTheta = std::cos(theta);
		return vector3<float>(fCosTheta * std::cos(phi), std::sin(theta), fCosTheta * std::sin(phi));
	}
	
	template<typename T>
	inline vector3<T>fromSphericalRotated(T theta, T phi)
	{
		T fSinTheta = std::sin(theta);
		return vector3<float>(fSinTheta * std::cos(phi), std::cos(theta), fSinTheta * std::sin(phi));
	}
	
	template <typename T>
	inline vector3<T> toSpherical(const vector3<T>& vec)
	{
		vector3<T> n = vec.normalized();
		return vector3<T>(std::atan2(n.z, n.x), std::asin(n.y), vec.length());
	}
	
	template <typename T>
	inline vector3<T> normalize(const vector3<T>& v)
		{ return v.normalized(); }
	
	template <typename T>
	inline vector3<T> cross(const vector3<T>& v1, const vector3<T>& v2)
		{ return v1.cross(v2); }
	
	template <typename T>
	inline T dot(const vector3<T>& v1, const vector3<T>& v2)
		{ return v1.dot(v2); }
	
	template <typename T>
	inline vector3<T> reflect(const vector3<T>& v, const vector3<T>& n)
		{ return v - static_cast<T>(2) * dot(v, n) * n; }
	
	template <typename T>
	inline T length(const vector3<T>& v)
		{ return v.length(); }
	
	template <typename T>
	inline vector3<float> vector3ToFloat(const vector3<T>& v)
		{ return vector3<float>(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)); }
}
