/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <Accelerate/Accelerate.h>

namespace et {

struct ET_ALIGNED(16) vec4simd {
public:
	vec4simd() {
	}

	explicit vec4simd(float v) :
		_data{ v, v, v, v } {
	}

	vec4simd(float x, float y, float z, float w) :
		_data{ x, y, z, w } {
	}

	vec4simd(const vector3<float>& v, float w) :
		_data{ v.x, v.y, v.z, w } {
	}

	explicit vec4simd(const vector4<float>& v) :
		_data{ v.x, v.y, v.z, v.w } {
	}

	explicit vec4simd(const vFloat& v) :
		_data(v) {
	}

	template <int c>
	float component() const {
		return _data[c];
	}

	float x() const {
		return component<0>();
	}

	float y() const {
		return component<1>();
	}

	float z() const {
		return component<2>();
	}

	float w() const {
		return component<3>();
	}

	const vector3<float> xyz() const {
		return vector3<float>(x(), y(), z());
	};

public:
	void clear() {
		_data = { 0.0f, 0.0f, 0.0f, 0.0f };
	}

	void addMultiplied(const vec4simd& r, float x) {
		_data += r._data * vFloat{ x, x, x, x };
	}

	void setMultiplied(const vec4simd& r, float x) {
		_data = r._data * vFloat{ x, x, x, x };
	}

	float divideByW() {
		float result = _data[3];
		_data = vdivf(_data, vFloat{ result, result, result, result });
		return result;
	}

	float dot(const vec4simd& r) const {
		float result = 0.0f;
		vDSP_dotpr(_vector, 1, r._vector, 1, &result, 4);
		return result;
	}

	vec4simd dotVector(const vec4simd& r) const {
		return vec4simd(dot(r));
	}

	float dotSelf() const {
		float result = 0.0f;
		vDSP_svesq(_vector, 1, &result, 4);
		return result;
	}

	vec4simd dotSelfVector() const {
		return vec4simd(dotSelf());
	}

	float length() const {
		return cblas_snrm2(4, _vector, 1);
	}

	void normalize() {
		cblas_sscal(4, 1.0f / length(), _vector, 1);
	}

	vec4simd inverseLengthVector() const {
		float l = length();
		return vec4simd(vrecf(vFloat{ l, l, l, l }));
	}

	vec4simd lengthVector() const {
		float l = length();
		return vec4simd(vFloat{ l, l, l, l });
	}

	vec4simd reciprocal() const {
		return vec4simd(vrecf(_data));
	}

	vec4simd crossXYZ(const vec4simd& r) const {
		return vec4simd(_data[1] * r._data[2] - _data[2] * r._data[1],
			_data[2] * r._data[0] - _data[0] * r._data[2],
			_data[0] * r._data[1] - _data[1] * r._data[0],
			0.0f);
	}

	template <int x, int y, int z, int w>
	vec4simd shuffle() const {
		return vec4simd(_data[x], _data[y], _data[z], _data[w]);
	}

	template <int x, int y, int z, int w>
	vec4simd shuffleWithoutOrdering() {
		return vec4simd(_data[w], _data[z], _data[y], _data[x]);
	}

	vec4simd floor() const {
		return vec4simd(vfloorf(_data));
	}

	bool firstComponentIsLessThan(const vec4simd& r) const {
		return false;
	}

	vec4simd sqrt() const {
		return vec4simd(vsqrtf(_data));
	}

public:
	vec4simd & operator += (const vec4simd& r) {
		_data += r._data;
		return *this;
	}

	vec4simd& operator -= (const vec4simd& r) {
		_data -= r._data;
		return *this;
	}

	vec4simd& operator *= (const vec4simd& r) {
		_data *= r._data;
		return *this;
	}

	vec4simd& operator /= (const vec4simd& r) {
		_data = vdivf(_data, r._data);
		return *this;
	}

	vec4simd& operator *= (float r) {
		_data *= vFloat{ r, r, r, r };
		return *this;
	}

	vec4simd& operator /= (float r) {
		_data = vdivf(_data, vFloat{ r, r, r, r });
		return *this;
	}

	vec4simd operator + (const vec4simd& r) const {
		return vec4simd(_data + r._data);
	}

	vec4simd operator - (const vec4simd& r) const {
		return vec4simd(_data - r._data);
	}

	vec4simd operator * (const vec4simd& r) const {
		return vec4simd(_data * r._data);
	}

	vec4simd operator / (const vec4simd& r) const {
		return vec4simd(vdivf(_data, r._data));
	}

	vec4simd operator * (float r) const {
		return vec4simd(_data * (vFloat{ r, r, r, r }));
	}

	vec4simd operator / (float r) const {
		return vec4simd(vdivf(_data, vFloat{ r, r, r, r }));
	}

public:
	void loadToVec4(vector4<float>& dst) const {
		memcpy(dst.data(), &_data, sizeof(vFloat));
	}

	void loadToFloats(float dst[4]) const {
		memcpy(dst, &_data, sizeof(vFloat));
	}

	void loadToFloatsUnaligned(float dst[4]) const {
		memcpy(dst, &_data, sizeof(vFloat));
	}

	vector4<float> toVec4() const {
		vector4<float> result(0.0f);
		loadToVec4(result);
		return result;
	}

private:
	union
	{
		vFloat _data;
		float _vector[4];
	} __attribute__((__aligned__(16)));
};
}
