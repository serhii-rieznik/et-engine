/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <arm_neon.h>

namespace et
{
	struct ET_ALIGNED(16) vec4simd
	{
	public:
		vec4simd() 
			{ }

		explicit vec4simd(float v) :
			_data(vdupq_n_f32(v)) { }

		vec4simd(float x, float y, float z, float w) :
			_data{x, y, z, w} { }

		vec4simd(const vector3<float>& v, float w) :
			_data{v.x, v.y, v.z, w} { }

		explicit vec4simd(const vector4<float>& v) :
			_data{v.x, v.y, v.z, v.w} { }
	
		explicit vec4simd(const float32x4_t& v) :
			_data(v) { }

		template <int c>
		float component() const 
		{
			return vgetq_lane_f32(_data, c);
		}

		float x() const
			{ return component<0>(); }

		float y() const
			{ return component<1>(); }

		float z() const 
			{ return component<2>(); }

		float w() const 
			{ return component<3>(); }

		const vector3<float> xyz() const
			{ return vector3<float>(x(), y(), z()); };
		
		const float32x4_t& data() const
			{ return _data; }
	
	public:
		void clear()
		{
			_data = { 0.0f, 0.0f, 0.0f, 0.0f };
		}

		void addMultiplied(const vec4simd& r, float x)
		{
			_data = vmlaq_f32(_data, r._data, float32x4_t{x, x, x, x});
		}

		void addMultiplied(const vec4simd& r, const vec4simd& q)
		{
			_data = vmlaq_f32(_data, r._data, q._data);
		}
		
		void setMultiplied(const vec4simd& r, float x)
		{
			float32x4_t s = {x, x, x, x};
			_data = r._data * s;
		}

		void setMultiplied(const vec4simd& r, const vec4simd& q)
		{
			_data = r._data * q._data;
		}

		void set_A_add_B_times_C(const vec4simd& a, const vec4simd& b, const vec4simd& c)
		{
			_data = vmlaq_f32(a._data, b._data, c._data);
		}
		
		float divideByW()
		{
			float result = w();
			_data = vdivq_f32(_data, vdupq_n_f32(result));
			return result;
		}

		float dot(const vec4simd& r) const
		{
			float32x4_t p = vmulq_f32(_data, r._data);
			float32x4_t s0 = vpaddq_f32(p, p);
			return vgetq_lane_f32(vpaddq_f32(s0, s0), 0);
		}

		vec4simd dotVector(const vec4simd& r) const
		{
			float32x4_t p = vmulq_f32(_data, r._data);
			float32x4_t s0 = vpaddq_f32(p, p);
			return vec4simd(vpaddq_f32(s0, s0));
		}

		float dotSelf() const
		{
			float32x4_t p = vmulq_f32(_data, _data);
			float32x4_t s0 = vpaddq_f32(p, p);
			return vgetq_lane_f32(vpaddq_f32(s0, s0), 0);
		}

		vec4simd dotSelfVector() const
		{
			float32x4_t p = vmulq_f32(_data, _data);
			float32x4_t s0 = vpaddq_f32(p, p);
			return vec4simd(vpaddq_f32(s0, s0));
		}

		float length() const
		{
			return ::sqrtf(dotSelf());
		}

		vec4simd lengthVector() const
		{
			float32x4_t p = vmulq_f32(_data, _data);
			float32x4_t s0 = vpaddq_f32(p, p);
			return vec4simd(vsqrtq_f32(vpaddq_f32(s0, s0)));
		}
		
		void normalize()
		{
			_data = vdivq_f32(_data, lengthVector()._data);
		}

		vec4simd inverseLengthVector() const
		{
			return vec4simd(1.0f / length());
		}

		vec4simd reciprocal() const
		{
			float32x4_t estimate = vrecpeq_f32(_data);
			estimate = vmulq_f32(vrecpsq_f32(estimate, _data), estimate);
			return vec4simd(vmulq_f32(vrecpsq_f32(estimate, _data), estimate));
		}

		vec4simd crossXYZ(const vec4simd& r) const
		{
			vec4simd s0 = shuffle<1, 2, 0, 3>() * r.shuffle<2, 0, 1, 3>();
			vec4simd s1 = shuffle<2, 0, 1, 3>() * r.shuffle<1, 2, 0, 3>();
			return s0 - s1;
		}

		template <int x, int y, int z, int w>
		vec4simd shuffle() const
		{
			return vec4simd(_data[x], _data[y], _data[z], _data[w]);
		}

		template <int x, int y, int z, int w>
		vec4simd shuffleWithoutOrdering()
		{
			return vec4simd(_data[w], _data[z], _data[y], _data[x]);
		}
		
		vec4simd floor() const
		{
			float32x4_t fi = vcvtq_f32_s32(vcvtq_s32_f32(_data));
			int32x4_t a = vandq_s32(vcgtq_f32(fi, _data), vdupq_n_s32(1));
			return vec4simd(vsubq_f32(fi, vcvtq_f32_s32(a)));
		}
		
		bool firstComponentIsLessThan(const vec4simd& r) const
		{
			return (_data[0] < r._data[0]);
		}
				
		vec4simd sqrt() const
		{
			return vec4simd(vsqrtq_f32(_data));
		}

	public:
		vec4simd& operator += (const vec4simd& r) 
		{
			_data += r._data;
			return *this;
		}

		vec4simd& operator -= (const vec4simd& r) 
		{
			_data -= r._data;
			return *this;
		}

		vec4simd& operator *= (const vec4simd& r) 
		{
			_data *= r._data;
			return *this;
		}

		vec4simd& operator /= (const vec4simd& r) 
		{
			_data /= r._data;
			return *this;
		}

		vec4simd& operator *= (float r) 
		{
			_data *= vdupq_n_f32(r);
			return *this;
		}

		vec4simd& operator /= (float r) 
		{
			_data /= vdupq_n_f32(r);
			return *this;
		}

		vec4simd operator + (const vec4simd& r) const 
		{
			return vec4simd(_data + r._data);
		}

		vec4simd operator - (const vec4simd& r) const
		{
			return vec4simd(_data - r._data);
		}

		vec4simd operator * (const vec4simd& r) const
		{
			return vec4simd(_data * r._data);
		}

		vec4simd operator / (const vec4simd& r) const
		{
			return vec4simd(_data * r._data);
		}

		vec4simd operator * (float r) const
		{
			return vec4simd(_data * vdupq_n_f32(r));
		}

		vec4simd operator / (float r) const
		{
			return vec4simd(_data / vdupq_n_f32(r));
		}

	public:
		void loadToVec4(vector4<float>& dst) const
		{
			vst1q_f32(dst.data(), _data);
		}

		void loadToFloats(float dst[4]) const
		{
			vst1q_f32(dst, _data);
		}

		void loadToFloatsUnaligned(float dst[4]) const
		{
			vst1q_f32(dst, _data);
		}

		vector4<float> toVec4() const
		{
			vector4<float> result(0.0f);
			loadToVec4(result);
			return result;
		}
		
	private:
		float32x4_t _data;
	};
}
