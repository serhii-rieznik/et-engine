/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/vector2.h>
#include <et/geometry/vector3.h>

namespace et
{
	template <int> struct Vector4AlignHelper { };

#if (ET_ENABLE_VEC4_ALIGN)
	template <> struct ET_ALIGNED( 4) Vector4AlignHelper<1> { };
	template <> struct ET_ALIGNED( 8) Vector4AlignHelper<2> { };
	template <> struct ET_ALIGNED(16) Vector4AlignHelper<4> { };
	template <> struct ET_ALIGNED(32) Vector4AlignHelper<8> { };
#endif

	template <typename T>
	struct vector4 : Vector4AlignHelper<sizeof(T)>
	{
	public:
		typedef T ComponentsType;
		
		enum : size_t
		{
			ComponentSize = sizeof(ComponentsType),
			ComponentsCount = 4,
			Size = ComponentsCount * ComponentSize
		};
		
	public:
		union
		{
			struct { T x, y, z, w; };
			T c[4];
		};
		
	public:
		vector4() 
#	if !defined(ET_DISABLE_VECTOR_INITIALIZATION)
			: x(T(0)), y(T(0)), z(T(0)), w(T(0))
#	endif
			{ }

		vector4(const vector4& c) :
			x(c.x), y(c.y), z(c.z), w(c.w) { }
		
		explicit vector4(T s) :
			x(s), y(s), z(s), w(s) { }

		vector4(T c, T a) :
			x(c), y(c), z(c), w(a) { }
		
		vector4(const vector2<T>& v1, const vector2<T>& v2) :
			x(v1.x), y (v1.y), z(v2.x), w(v2.y) { }

		vector4(const vector2<T>& v2, T _z, T _w) :
			x(v2.x), y(v2.y), z(_z), w(_w) { }

		vector4(const vector3<T>& v3, T _w) :
			x(v3.x), y(v3.y), z(v3.z), w(_w) { }

		explicit vector4(const vector3<T>& v3) :
			x(v3.x), y(v3.y), z(v3.z), w(0) { }

		vector4(T _x, T _y, T _z, T _w) :
			x(_x), y(_y), z(_z), w(_w) { }

		T* data()
			{ return c; }

		const T* data() const 
			{ return c; }

		char* binary()
			{ return reinterpret_cast<char*>(c); }

		const char* binary() const
			{ return reinterpret_cast<const char*>(c); }

		T& operator[](int32_t i)
			{ return c[i] ;}

		const T& operator[](int32_t i) const
			{ return c[i] ;}

		T& operator[](uint32_t i)
			{ return c[i] ;}

		const T& operator[](uint32_t i) const
			{ return c[i] ;}

		vector4 operator -()
			{ return vector4(-x, -y, -z, -w); }

		const vector4 operator -() const
			{ return vector4(-x, -y, -z, -w); }

		vector4 operator * (T v) const
			{ return vector4(x * v, y * v, z * v, w * v); }

		vector4 operator / (T v) const
			{ return vector4(x / v, y / v, z / v, w / v); }

		vector4 operator + (const vector4& value) const
			{ return vector4(x + value.x, y + value.y, z + value.z, w + value.w); }

		vector4 operator - (const vector4& value) const
			{ return vector4(x - value.x, y - value.y, z - value.z, w - value.w); }

		vector4 operator * (const vector4& value) const
			{ return vector4(x * value.x, y * value.y, z * value.z, w * value.w); }

		vector4 operator / (const vector4& value) const
			{ return vector4(x / value.x, y / value.y, z / value.z, w / value.w); }

		vector4& operator += (const vector4 &value)
			{ x += value.x; y += value.y; z += value.z; w += value.w; return *this; }

		vector4& operator -= (const vector4 &value)
			{ x -= value.x; y -= value.y; z -= value.z; w -= value.w; return *this; }

		vector4& operator *= (const vector4 &value)
			{ x *= value.x; y *= value.y; z *= value.z; w *= value.w; return *this; }

		vector4& operator /= (const vector4 &value)
			{ x /= value.x; y /= value.y; z /= value.z; w /= value.w; return *this; }

		vector4& operator *= (T value)
			{ x *= value; y *= value; z *= value; w *= value; return *this; }

		vector4& operator /= (T value)
			{ x /= value; y /= value; z /= value; w /= value; return *this; }

		vector2<T>& xy()
			{ return *(reinterpret_cast<vector2<T>*>(c)); }

		vector3<T>& xyz()
			{ return *(reinterpret_cast<vector3<T>*>(c)); }

		const vector2<T>& xy() const
			{ return *(reinterpret_cast<const vector2<T>*>(c)); }

		const vector3<T>& xyz() const
			{ return *(reinterpret_cast<const vector3<T>*>(c)); }

		const vector2<T>& zw() const
			{ return *(reinterpret_cast<vector2<T>*>(c+2)); }
		
		T dot(const vector4& vector) const
			{ return x*vector.x + y*vector.y + z*vector.z + w*vector.w; }

		T dotSelf() const
			{ return x*x + y*y + z*z + w*w; }

		bool operator == (const vector4& r) const
			{ return (x == r.x) && (y == r.y) && (z == r.z) && (w == r.w); }

		bool operator != (const vector4& r) const
			{ return (x != r.x) || (y != r.y) || (z != r.z) || (w != r.w); }

		void addMultiplied(const vector4<T>& m, T a)
		{
			x += a * m.x;
			y += a * m.y;
			z += a * m.z;
			w += a * m.w;
		}

		void setMultiplied(const vector4<T>& m, T a)
		{
			x = a * m.x;
			y = a * m.y;
			z = a * m.z;
			w = a * m.w;
		}

		void clear()
		{
			x = 0;
			y = 0;
			z = 0;
			w = 0;
		}

		void set(float value)
		{
			x = value;
			y = value;
			z = value;
			w = value;
		}

		void divideXYZByW()
		{
			x /= w;
			y /= w;
			z /= w;
		}

		float divideByW()
		{
			float r = w;
			x /= w;
			y /= w;
			z /= w;
			w = 1.0f;
			return r;
		}

		void normalize()
		{
			T l = dotSelf();
			if (l > std::numeric_limits<float>::epsilon())
			{
				l = std::sqrt(l);
				x /= l;
				y /= l;
				z /= l;
				w /= l;
			}
		}

		float length() const 
			{ return std::sqrt(dotSelf()); }

		template <int x, int y, int z, int w>
		vector4<T> shuffle() const
			{ return vector4<T>(c[x], c[y], c[z], c[w]); }

		vector4<T> reciprocal() const
			{ return vector4<T>(1.0f / x, 1.0f / y, 1.0f / z, 1.0f / w); }
		
		vector4<T> crossXYZ(const vector4<T>& vec) const
		{
			return vector4
			(
				y * vec.z - z * vec.y,
				z * vec.x - x * vec.z,
				x * vec.y - y * vec.x,
				0.0f
			);
		}
		
		vector4<T> minWith(const vector4<T>& vec) const
		{
			return vector4
			(
				(x < vec.x) ? x : vec.x,
				(y < vec.y) ? y : vec.y,
				(z < vec.z) ? z : vec.z,
				(w < vec.w) ? w : vec.w
			);
		}
		
		vector4<T> maxWith(const vector4<T>& vec) const
		{
			return vector4
			(
				(x > vec.x) ? x : vec.x,
				(y > vec.y) ? y : vec.y,
				(z > vec.z) ? z : vec.z,
				(w > vec.w) ? w : vec.w
			 );
		}
		
		vector4<T> dotVector(const vector4<T>& vec) const
		{
			float dotProduct = x * vec.x + y * vec.y + z * vec.z + w * vec.w;
			return vector4(dotProduct, dotProduct, dotProduct, dotProduct);
		}
		
		vector4<float> toVec4() const
		{
			return vector4<float>(static_cast<float>(x), static_cast<float>(y),
				static_cast<float>(z), static_cast<float>(w));
		}
		
		void loadToFloats(float* data) const
		{
			static_assert(std::is_same<T, float>::value, "Only works for float vector");
			memcpy(data, c, sizeof(float) * 4);
		}

		void loadToVec4(vector4<float>& target) const
		{
			static_assert(std::is_same<T, float>::value, "Only works for float vector");
			target = *this;
		}
		
		float cX() const { return x; }
		float cY() const { return y; }
		float cZ() const { return z; }
		float cW() const { return w; }
	};

	template <typename T>
	inline vector4<T> operator * (T value, const vector4<T>& vec)
		{ return vector4<T>(vec.x * value, vec.y * value, vec.z * value, vec.w * value); }
}
