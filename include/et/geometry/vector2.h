/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

namespace et 
{
	template <typename T> 
	struct vector2
	{
		union
		{
			struct { T x, y; };
			T c[2];
		};

		vector2() :
			x(static_cast<T>(0)), y(static_cast<T>(0)) { }
		
		explicit vector2(T value) :
			x(value), y(value) { }
		
		vector2(T _x, T _y) :
			x(_x), y(_y) { }

		T& operator [](int i)
			{ return c[i]; }

		const T& operator [](int i) const
			{ return c[i]; }

		T& operator [](size_t i)
			{ return c[i]; }

		const T& operator [](size_t i) const
			{ return c[i]; }

		T* data() 
			{ return c; }

		const T* data() const
			{ return c; }

		char* binary()
			{ return reinterpret_cast<char*>(c); }

		const char* binary() const
			{ return reinterpret_cast<const char*>(c); }

		bool operator == (const vector2& value) const
			{ return (value.x == x) && (value.y == y); }

		bool operator != (const vector2& value) const
			{ return (value.x != x) || (value.y != y); }

		vector2 operator - () const
			{ return vector2(-x, -y); }

		vector2 operator + (const vector2& value) const
			{ return vector2(x + value.x, y + value.y); };

		vector2 operator - (const vector2& value) const
			{ return vector2(x - value.x, y - value.y); };

		vector2 operator * (const vector2& value) const
			{ return vector2(x * value.x, y * value.y); };

		vector2 operator / (const vector2& value) const
			{ return vector2(x / value.x, y / value.y); };

		vector2 operator * (const T& value) const
			{ return vector2(x * value, y * value); };

		vector2 operator / (const T& value) const
			{ return vector2(x / value, y / value); };

		vector2& operator += (const vector2 &value)
			{ x += value.x; y += value.y; return *this; }

		vector2& operator -= (const vector2 &value)
			{ x -= value.x; y -= value.y; return *this; }

		vector2& operator *= (const vector2 &value)
			{ x *= value.x; y *= value.y; return *this; }

		vector2& operator /= (const vector2 &value)
			{ x /= value.x; y /= value.y; return *this; }
		
		vector2& operator *= (T value)
			{ x *= value; y *= value; return *this; }

		vector2& operator /= (T value)
			{ x /= value; y /= value; return *this; }
		
		T dotSelf() const 
			{ return x*x + y*y; }

		T dot(vector2<T>& v) const
			{ return x * v.x + y * v.y; }

		T length() const
            { return std::sqrt(dotSelf()); }

		T square() const
			{ return x * y; }

		T aspect() const
			{ return x / y; }

		vector2& normalize()
		{
			T lenSquare = dotSelf();
			if (lenSquare > 0)
			{
				lenSquare = std::sqrt(lenSquare);
				x /= lenSquare;
				y /= lenSquare;
			}
			return *this;
		}
	};

	template <typename T>
	vector2<T> operator * (const vector2<T>& vec, T value)
		{ return vector2<T>(vec.x * value, vec.y * value); }

	template <typename T>
	vector2<T> operator * (T value, const vector2<T>& vec) 
		{ return vector2<T>(vec.x * value, vec.y * value); }

}