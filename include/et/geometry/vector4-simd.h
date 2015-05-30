/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <xmmintrin.h>
#include <et/geometry/vector3.h>

namespace et
{
	struct ET_ALIGNED(16) vec4simd
	{
	public:
		vec4simd() 
			{ }

		explicit vec4simd(float v) : 
			_data(_mm_set_ps1(v)) { }

		vec4simd(float x, float y, float z, float w) : 
			_data(_mm_set_ps(w, z, y, x)) { }

		vec4simd(const vector3<float>& v, float w) : 
			_data(_mm_set_ps(w, v.z, v.y, v.x)) { }

		explicit vec4simd(const vector4<float>& v) : 
			_data(_mm_loadu_ps(v.data())) { }

		explicit vec4simd(__m128 i) : 
			_data(i) { }

		template <int c>
		float component() const 
		{
			auto shuffled = _mm_shuffle_ps(_data, _data, _MM_SHUFFLE(c, c, c, c));
			float result;
			_mm_store_ss(&result, shuffled);
			return result;
		}

		float x() const
			{ return component<0>(); }

		float y() const
			{ return component<1>(); }

		float z() const 
			{ return component<2>(); }

		float w() const 
			{ return component<3>(); }

	public:
		void clear() 
		{
			_data = _mm_setzero_ps();
		}

		void addMultiplied(const vec4simd& r, float x)
		{
			__m128 scalar = _mm_set_ps1(x);
			scalar = _mm_mul_ps(scalar, r._data);
			_data = _mm_add_ps(_data, scalar);
		}

		void setMultiplied(const vec4simd& r, float x)
		{
			__m128 scalar = _mm_set_ps1(x);
			_data = _mm_mul_ps(scalar, r._data);
		}

		float divideByW()
		{
			float result;
			__m128 w = _mm_shuffle_ps(_data, _data, _MM_SHUFFLE(3, 3, 3, 3));
			_mm_store_ss(&result, w);
			_data = _mm_div_ps(_data, w);
			return result;
		}

		const vector3<float> xyz() const
			{ return vector3<float>(x(), y(), z()); };

		float dotSelf() const
		{
			__m128 s0 = _mm_mul_ps(_data, _data);
			__m128 s1 = _mm_hadd_ps(s0, s0);
			__m128 s2 = _mm_hadd_ps(s1, s1);
			float result;
			_mm_store_ss(&result, s2);
			return result;
		}

		float length() const
		{
			__m128 s0 = _mm_mul_ps(_data, _data);
			__m128 s1 = _mm_hadd_ps(s0, s0);
			__m128 s2 = _mm_hadd_ps(s1, s1);
			__m128 s3 = _mm_sqrt_ss(s2);
			float result;
			_mm_store_ss(&result, s3);
			return result;
		}

		void normalize()
		{
			__m128 s0 = _mm_mul_ps(_data, _data);
			__m128 s1 = _mm_hadd_ps(s0, s0);
			__m128 s2 = _mm_hadd_ps(s1, s1);
			__m128 s3 = _mm_rsqrt_ps(s2);
			_data = _mm_mul_ps(_data, s3);
		}

		vec4simd inverseSqrtVector() const
		{
			__m128 s0 = _mm_mul_ps(_data, _data);
			__m128 s1 = _mm_hadd_ps(s0, s0);
			__m128 s2 = _mm_hadd_ps(s1, s1);
			return vec4simd(_mm_rsqrt_ps(s2));
		}

		vec4simd lengthVector() const
		{
			__m128 s0 = _mm_mul_ps(_data, _data);
			__m128 s1 = _mm_hadd_ps(s0, s0);
			__m128 s2 = _mm_hadd_ps(s1, s1);
			return vec4simd(_mm_sqrt_ss(s2));
		}

		vec4simd reciprocal() const 
		{
			return vec4simd(_mm_rcp_ps(_data));
		}

	public:
		vec4simd& operator += (const vec4simd& r) 
		{
			_data = _mm_add_ps(_data, r._data);
			return *this;
		}

		vec4simd& operator -= (const vec4simd& r) 
		{
			_data = _mm_sub_ps(_data, r._data);
			return *this;
		}

		vec4simd& operator *= (const vec4simd& r) 
		{
			_data = _mm_mul_ps(_data, r._data);
			return *this;
		}

		vec4simd& operator /= (const vec4simd& r) 
		{
			_data = _mm_div_ps(_data, r._data);
			return *this;
		}

		vec4simd& operator *= (float r) 
		{
			__m128 scalar = _mm_set_ps1(r);
			_data = _mm_mul_ps(_data, scalar);
			return *this;
		}

		vec4simd& operator /= (float r) 
		{
			__m128 scalar = _mm_set_ps1(r);
			_data = _mm_div_ps(_data, scalar);
			return *this;
		}

		vec4simd operator + (const vec4simd& r) const 
		{
			return vec4simd(_mm_add_ps(_data, r._data));
		}

		vec4simd operator - (const vec4simd& r) const
		{
			return vec4simd(_mm_sub_ps(_data, r._data));
		}

		vec4simd operator * (const vec4simd& r) const
		{
			return vec4simd(_mm_mul_ps(_data, r._data));
		}

	public:
		void loadToVec4(vector4<float>& dst) const
		{
			_mm_storeu_ps(dst.data(), _data);
		}

		vector4<float> toVec4() const
		{
			vector4<float> result;
			loadToVec4(result);
			return result;
		}

	private:
		__m128 _data;
	};
}
