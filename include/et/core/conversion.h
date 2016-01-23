/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/geometry.h>

namespace et
{
	vec2 strToVector2(const std::string& s, const std::string& delimiter = std::string(ET_DEFAULT_DELIMITER_STRING));
	vec3 strToVector3(const std::string& s, const std::string& delimiter = std::string(ET_DEFAULT_DELIMITER_STRING));
	vec4 strToVector4(const std::string& s, const std::string& delimiter = std::string(ET_DEFAULT_DELIMITER_STRING));
	vec4 strHexToVec4(const std::string& s);
	vec4 strHexToVec4(const std::wstring& s);
		
	ArrayValue vec2ToArray(const vec2&);
	ArrayValue vec3ToArray(const vec3&);
	ArrayValue vec4ToArray(const vec4&);
	ArrayValue rectToArray(const rect&);
	ArrayValue quaternionToArray(const quaternion&);
	ArrayValue matrixToArray(const mat4&);

	vec2 arrayToVec2(ArrayValue);
	vec2i arrayToVec2i(ArrayValue);
	
	vec3 arrayToVec3(ArrayValue);
	vec4 arrayToVec4(ArrayValue);
	rect arrayToRect(ArrayValue);
	quaternion arrayToQuaternion(ArrayValue);
	mat4 arrayToMatrix(ArrayValue);
}
