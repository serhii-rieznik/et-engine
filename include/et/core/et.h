/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <limits>
#include <functional>
#include <algorithm>

#include <iosfwd>
#include <iostream>

#define ET_MAJOR_VERSION		0
#define ET_MINOR_VERSION		4

#define ET_CORE_INCLUDES

#include <et/platform/compileoptions.h>
#include <et/platform/platform.h>

#include <et/core/debug.h>
#include <et/core/constants.h>
#include <et/core/memory.h>
#include <et/core/properties.h>
#include <et/core/strings.h>
#include <et/core/memoryallocator.h>

#include <et/core/autoptr.h>
#include <et/core/atomiccounter.h>
#include <et/core/intrusiveptr.h>

#include <et/core/filesystem.h>
#include <et/core/conversionbase.h>
#include <et/core/object.h>
#include <et/core/dictionary.h>
#include <et/core/stream.h>
#include <et/core/hierarchy.h>
#include <et/core/log.h>

#include <et/geometry/vector4.h>
#include <et/geometry/matrix3.h>
#include <et/geometry/matrix4.h>
#include <et/geometry/quaternion.h>
#include <et/geometry/rect.h>

#undef ET_CORE_INCLUDES

namespace et
{
	typedef vector2<float> vec2;
	typedef vector3<float> vec3;
	typedef vector4<float> vec4;
	
	typedef vector2<int> vec2i;
	typedef vector3<int> vec3i;
	typedef vector4<int> vec4i;
	
	typedef vector2<size_t> vec2sz;
	typedef vector3<size_t> vec3sz;
	typedef vector4<size_t> vec4sz;
	
	typedef vector2<unsigned char> vec2ub;
	typedef vector3<unsigned char> vec3ub;
	typedef vector4<unsigned char> vec4ub;
	
	typedef matrix3<float> mat3;
	typedef matrix4<float> mat4;
	
	typedef matrix3<int> mat3i;
	typedef matrix4<int> mat4i;
	
	typedef Quaternion<float> quaternion;
	
	typedef Rect<float> rect;
	typedef Rect<int> recti;
	
	template <typename T>
	inline T etMin(const T& v1, const T& v2)
		{ return (v1 < v2) ? v1 : v2; }
	
	template <typename T>
	inline T etMax(const T& v1, const T& v2)
		{ return (v1 > v2) ? v1 : v2; }

	template<typename T>
	inline T clamp(T value, T min, T max)
		{ return (value < min) ? min : (value > max) ? max : value; }
	
	extern const vec3 unitX;
	extern const vec3 unitY;
	extern const vec3 unitZ;
	extern const mat3 identityMatrix3;
	extern const mat4 identityMatrix;
	extern const mat4 lightProjectionMatrix;
}
