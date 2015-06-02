/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#if defined(__SSE4_1__)
#	include <smmintrin.h>
#else
#	include <pmmintrin.h>
#endif

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
			return _mm_cvtss_f32(_mm_shuffle_ps(_data, _data, _MM_SHUFFLE(c, c, c, c)));
		}

		float x() const
			{ return _mm_cvtss_f32(_data); }

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

		float dot(const vec4simd& r) const
		{
			return _mm_cvtss_f32(dotImpl(_data, r._data, 0x71));
		}

		vec4simd dotVector(const vec4simd& r) const
		{
			return vec4simd(dotImpl(_data, r._data, 0x71));
		}

		float dotSelf() const
		{
			return _mm_cvtss_f32(dotImpl(_data, _data, 0x71));
		}

		vec4simd dotSelfVector() const
		{
			return vec4simd(dotImpl(_data, _data, 0x71));
		}

		float length() const
		{
			__m128 s0 = dotImpl(_data, _data, 0x71);
			return _mm_cvtss_f32(_mm_sqrt_ss(s0));
		}

		void normalize()
		{
			__m128 norm = _mm_sqrt_ps(dotImpl(_data, _data, 0x7F));
			_data = _mm_div_ps(_data, norm);		
		}

		vec4simd inverseLengthVector() const
		{
			__m128 s0 = dotImpl(_data, _data, 0x7F);
			return vec4simd(_mm_rsqrt_ps(s0));
		}

		vec4simd lengthVector() const
		{
			__m128 s0 = dotImpl(_data, _data, 0x71);
			return vec4simd(_mm_sqrt_ss(s0));
		}

		vec4simd reciprocal() const 
		{
			return vec4simd(_mm_rcp_ps(_data));
		}

		vec4simd crossXYZ(const vec4simd& r) const
		{
			__m128 result = _mm_sub_ps
			( 
				_mm_mul_ps(_data, _mm_shuffle_ps(r._data, r._data, _MM_SHUFFLE(3, 0, 2, 1))), 
				_mm_mul_ps(r._data, _mm_shuffle_ps(_data, _data, _MM_SHUFFLE(3, 0, 2, 1))) 
			);
			return vec4simd(_mm_shuffle_ps(result, result, _MM_SHUFFLE(3, 0, 2, 1 )));
		}

		vec4simd maxWith(const vec4simd& r) const
		{
			return vec4simd(_mm_max_ps(_data, r._data));
		}

		template <int x, int y, int z, int w>
		vec4simd shuffle() const
		{
			return vec4simd(_mm_shuffle_ps(_data, _data, _MM_SHUFFLE(w, z, y, x)));
		}
		
		template <int i1, int i2, int i3, int i4>
		vec4simd shuffleWithoutOrdering() const
		{
			return vec4simd(_mm_shuffle_ps(_data, _data, _MM_SHUFFLE(i1, i2, i3, i4)));
		}
		
		vec4simd floor() const
		{
			return vec4simd(floorImpl(_data));
		}
		
		bool firstComponentIsLessThan(const vec4simd& r) const
		{
			return _mm_comilt_ss(_data, r._data);
		}
		
		vec4simd sqrtFirstComponent() const
		{
			return vec4simd(_mm_sqrt_ss(_data));
		}

		vec4simd sqrt() const
		{
			return vec4simd(_mm_sqrt_ps(_data));
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

		vec4simd operator / (const vec4simd& r) const
		{
			return vec4simd(_mm_div_ps(_data, r._data));
		}

		vec4simd operator * (float r) const
		{
			return vec4simd(_mm_mul_ps(_data, _mm_set_ps1(r)));
		}

		vec4simd operator / (float r) const
		{
			return vec4simd(_mm_div_ps(_data, _mm_set_ps1(r)));
		}

	public:
		void loadToVec4(vector4<float>& dst) const
		{
			_mm_storeu_ps(dst.data(), _data);
		}

		void loadToFloats(float dst[4]) const
		{
			_mm_store_ps(dst, _data);
		}

		void loadToFloatsUnaligned(float dst[4]) const
		{
			_mm_storeu_ps(dst, _data);
		}

		vector4<float> toVec4() const
		{
			vector4<float> result;
			loadToVec4(result);
			return result;
		}

		const __m128& data() const 
			{ return _data; }

	private:
		__m128 dotImpl(const __m128& a, const __m128& b, int mask) const
		{
#		if defined(__SSE4_1__)
			return _mm_dp_ps(a, b, 0x71);
#		else
			__m128 dp = _mm_mul_ps(_data, _data);
			__m128 s1 = _mm_hadd_ps(dp, dp);
			return _mm_hadd_ps(s1, s1);
#		endif
		}
		
		__m128 floorImpl(__m128 x) const
		{
#		if defined(_mm_floor_ps)
			return _mm_floor_ps(x);
#		else
			__m128 fi = _mm_cvtepi32_ps(_mm_cvttps_epi32(x));
			return _mm_sub_ps(fi, _mm_and_ps(_mm_cmpgt_ps(fi, x), _mm_set_ps1(1.0f)));
#		endif
		}
		
	private:
		__m128 _data;
	};
}
