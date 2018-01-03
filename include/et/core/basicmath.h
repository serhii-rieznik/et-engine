/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

namespace et {

template<typename T>
inline T sqr(T value) {
	return value * value;
}

inline float length(float v) {
	return std::abs(v);
}

inline float mix(float v1, float v2, float t) {
	return v1 * (1.0f - t) + v2 * t;
}

inline float lerp(float v1, float v2, float t) {
	return v1 * (1.0f - t) + v2 * t;
}

inline void normalizeAngle(float& angle) {
	while (angle < -PI) angle += DOUBLE_PI;
	while (angle > PI) angle -= DOUBLE_PI;
}

template <typename T>
inline T intPower(T value, size_t power) {
	T result = static_cast<T>(1);
	for (size_t i = 1; i <= power; ++i)
		result *= value;
	return result;
}

template <typename T>
T bezierCurve(T* points, size_t size, float time) {
	ET_ASSERT((size > 0) && (points != nullptr));

	if (size == 1)
		return *points;

	if (size == 2)
		return mix(*points, points[1], time);

	if (size == 3)
	{
		float invTime = 1.0f - time;
		return sqr(invTime) * points[0] + 2.0f * invTime * time * points[1] + sqr(time) * points[2];
	}

	return mix(bezierCurve<T>(points, size - 1, time), bezierCurve<T>(points + 1, size - 1, time), time);
}

}
