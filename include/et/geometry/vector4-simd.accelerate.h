/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <Accelerate/Accelerate.h>

namespace et
{
	struct ET_ALIGNED(16) vec4simd
	{
	public:
		vec4simd() 
			{ }

		explicit vec4simd(float v)
			{ }

		vec4simd(float x, float y, float z, float w)
			{ }

		vec4simd(const vector3<float>& v, float w)
			{ }

		explicit vec4simd(const vector4<float>& v)
			{ }

		template <int c>
		float component() const 
		{
			return 0.0f;
		}

		float x() const
			{ return 0.0f; }

		float y() const
			{ return component<1>(); }

		float z() const 
			{ return component<2>(); }

		float w() const 
			{ return component<3>(); }

	public:
		void clear()
		{
		}

		void addMultiplied(const vec4simd& r, float x)
		{
		}

		void setMultiplied(const vec4simd& r, float x)
		{
		}

		float divideByW()
		{
			return 1.0f;
		}

		const vector3<float> xyz() const
			{ return vector3<float>(x(), y(), z()); };

		float dot(const vec4simd& r) const
		{
			return 0.0f;
		}

		vec4simd dotVector(const vec4simd& r) const
		{
			return vec4simd();
		}

		float dotSelf() const
		{
			return 0.0f;
		}

		vec4simd dotSelfVector() const
		{
			return vec4simd();
		}

		float length() const
		{
			return 0.0f;
		}

		void normalize()
		{
		}

		vec4simd inverseLengthVector() const
		{
			return vec4simd();
		}

		vec4simd lengthVector() const
		{
			return vec4simd();
		}

		vec4simd reciprocal() const 
		{
			return vec4simd();
		}

		vec4simd crossXYZ(const vec4simd& r) const
		{
			return vec4simd();
		}

		vec4simd maxWith(const vec4simd& r) const
		{
			return vec4simd();
		}

		template <int x, int y, int z, int w>
		vec4simd shuffle() const
		{
			return vec4simd();
		}

		template <int x, int y, int z, int w>
		vec4simd shuffleWithoutOrdering()
		{
			return vec4simd();
		}
		
		vec4simd floor() const
		{
			return vec4simd();
		}
		
		bool firstComponentIsLessThan(const vec4simd& r) const
		{
			return false;
		}
		
		vec4simd sqrtFirstComponent() const
		{
			return vec4simd();
		}
		
		vec4simd sqrt() const
		{
			return vec4simd();
		}

	public:
		vec4simd& operator += (const vec4simd& r) 
		{
			return *this;
		}

		vec4simd& operator -= (const vec4simd& r) 
		{
			return *this;
		}

		vec4simd& operator *= (const vec4simd& r) 
		{
			return *this;
		}

		vec4simd& operator /= (const vec4simd& r) 
		{
			return *this;
		}

		vec4simd& operator *= (float r) 
		{
			return *this;
		}

		vec4simd& operator /= (float r) 
		{
			return *this;
		}

		vec4simd operator + (const vec4simd& r) const 
		{
			return vec4simd();
		}

		vec4simd operator - (const vec4simd& r) const
		{
			return vec4simd();
		}

		vec4simd operator * (const vec4simd& r) const
		{
			return vec4simd();
		}

		vec4simd operator / (const vec4simd& r) const
		{
			return vec4simd();
		}

		vec4simd operator * (float r) const
		{
			return vec4simd();
		}

		vec4simd operator / (float r) const
		{
			return vec4simd();
		}

	public:
		void loadToVec4(vector4<float>& dst) const
		{
		}

		void loadToFloats(float dst[4]) const
		{
		}

		void loadToFloatsUnaligned(float dst[4]) const
		{
		}

		vector4<float> toVec4() const
		{
			vector4<float> result(0.0f);
			return result;
		}
	};
}
