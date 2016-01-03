/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/conversion.h>

namespace et
{
	template <typename R, int C>
	R strToVector(const std::string& s, const std::string& delimiter)
	{
		R result;
		int index = 0;
		StringList values = split(s, delimiter);
		for (auto& i : values)
		{
			result[index++] = strToFloat(i);
			if (index >= C) break;
		}
		return result;
	}
}

using namespace et;

std::string et::floatToTimeStr(float value, bool showMSec)
{
	int seconds = static_cast<int>(value);
	int mSec = static_cast<int>((value - static_cast<float>(seconds)) * 1000.0f);

	int hours = seconds / 3600;
	seconds -= 3600 * hours;
	int minutes = seconds / 60;
	seconds -= minutes * 60;
	
	hours = std::abs(hours);
	minutes = std::abs(minutes);
	seconds = std::abs(seconds);
	mSec = std::abs(mSec);
	
	std::string sMin = intToStr(minutes);
	std::string sSec = intToStr(seconds);
	std::string sMSec = intToStr(mSec);

	if ((hours > 0) && (minutes < 10))
		sMin = "0" + sMin;
	
	if (seconds < 10)
		sSec = "0" + sSec;

	if (showMSec)
	{
		if (mSec < 100)
			sMSec = "0" + sMSec;
		if (mSec < 10)
			sMSec = "0" + sMSec;
	}

	std::string result;

	if (hours > 0)
	{
		if (hours > 24)
		{
			auto days = hours / 24;
			hours -= 24 * days;
			
			result = intToStr(days) + ", " + intToStr(hours) + ":" + sMin + ":" + sSec;
		}
		else
		{
			result = intToStr(hours) + ":" + sMin + ":" + sSec;
		}
	}
	else
		result = sMin + ":" + sSec;

	if (showMSec)
		result += ":" + sMSec;
	
	if (value < 0)
		result = "-" + result;

	return result;
}

std::string et::floatToStr(float v, int precission)
{ 
	char format[16] = { };
	sprintf(format, "%%.%df", precission);
	
	char buffer[256] = { };
	sprintf(buffer, format, v);
	
	return std::string(buffer);
}

vec2 et::strToVector2(const std::string& s, const std::string& delimiter)
	{ return strToVector<vec2, 2>(s, delimiter); }

vec3 et::strToVector3(const std::string& s, const std::string& delimiter)
	{ return strToVector<vec3, 3>(s, delimiter); }

vec4 et::strToVector4(const std::string& s, const std::string& delimiter)
	{ return strToVector<vec4, 4>(s, delimiter); }

int et::hexCharacterToInt(int c)
{
	c = tolower(c);
	
	if ((c >= '0') && (c <= '9')) 
		return c - '0';
	else if ((c >= 'a') && (c <= 'f')) 
		return c - 'a' + 10;
	else 
		return 0;
}

vec4 et::strHexToVec4(const std::string& s)
{
	vec4 result;
	uint32_t value = 0;
	int l = etMin(8, static_cast<int>(s.size()));

	uint32_t scale = 1;
	const char* cstr = s.c_str();
	for (int i = l - 1; i >= 0; --i)
	{
		value += scale * static_cast<uint32_t>(hexCharacterToInt(tolower(cstr[i])));
		scale *= 16;
	}

	unsigned char a = static_cast<unsigned char>((value & 0xff000000) >> 24);
	unsigned char b = static_cast<unsigned char>((value & 0x00ff0000) >> 16);
	unsigned char g = static_cast<unsigned char>((value & 0x0000ff00) >> 8);
	unsigned char r = static_cast<unsigned char>((value & 0x000000ff) >> 0);

	result.x = static_cast<float>(r) / 255.0f;
	result.y = static_cast<float>(g) / 255.0f;
	result.z = static_cast<float>(b) / 255.0f;
	result.w = static_cast<float>(a) / 255.0f;

	return result;
}

vec4 et::strHexToVec4(const std::wstring& s)
{
	vec4 result;
	uint32_t value = 0;
	int l = etMin(8, static_cast<int>(s.size()));

	uint32_t scale = 1;
	const wchar_t* cstr = s.c_str();
	for (int i = l - 1; i >= 0; --i)
	{
		value += scale * static_cast<uint32_t>(hexCharacterToInt(::tolower(cstr[i])));
		scale *= 16;
	}
	
	unsigned char a = static_cast<unsigned char>((value & 0xff000000) >> 24);
	unsigned char b = static_cast<unsigned char>((value & 0x00ff0000) >> 16);
	unsigned char g = static_cast<unsigned char>((value & 0x0000ff00) >> 8);
	unsigned char r = static_cast<unsigned char>((value & 0x000000ff) >> 0);

	result.x = static_cast<float>(r) / 255.0f;
	result.y = static_cast<float>(g) / 255.0f;
	result.z = static_cast<float>(b) / 255.0f;
	result.w = static_cast<float>(a) / 255.0f;

	return result;
}

ArrayValue et::vec2ToArray(const vec2& v)
{
	ArrayValue result;
	result->content.push_back(FloatValue(v.x));
	result->content.push_back(FloatValue(v.y));
	return result;
}

ArrayValue et::vec3ToArray(const vec3& v)
{
	ArrayValue result;
	result->content.push_back(FloatValue(v.x));
	result->content.push_back(FloatValue(v.y));
	result->content.push_back(FloatValue(v.z));
	return result;
}

ArrayValue et::vec4ToArray(const vec4& v)
{
	ArrayValue result;
	result->content.push_back(FloatValue(v.x));
	result->content.push_back(FloatValue(v.y));
	result->content.push_back(FloatValue(v.z));
	result->content.push_back(FloatValue(v.w));
	return result;
}

ArrayValue et::rectToArray(const rect& v)
{
	ArrayValue result;
	result->content.push_back(FloatValue(v.left));
	result->content.push_back(FloatValue(v.top));
	result->content.push_back(FloatValue(v.width));
	result->content.push_back(FloatValue(v.height));
	return result;
}

ArrayValue et::quaternionToArray(const quaternion& q)
{
	ArrayValue result;
	result->content.push_back(FloatValue(q.scalar));
	result->content.push_back(FloatValue(q.vector.x));
	result->content.push_back(FloatValue(q.vector.y));
	result->content.push_back(FloatValue(q.vector.z));
	return result;
}

vec2 et::arrayToVec2(ArrayValue a)
{
	vec2 result;
	int index = 0;
	for (auto v : a->content)
	{
		if (v->valueClass() == ValueClass_Float)
			result[index++] = FloatValue(v)->content;
		else if (v->valueClass() == ValueClass_Integer)
			result[index++] = static_cast<float>(IntegerValue(v)->content);
		
		if (index >= 2) break;
	}
	return result;
}

vec2i et::arrayToVec2i(ArrayValue a)
{
	vec2i result;
	int index = 0;
	for (auto v : a->content)
	{
		if (v->valueClass() == ValueClass_Float)
			result[index++] = static_cast<int>(FloatValue(v)->content);
		else if (v->valueClass() == ValueClass_Integer)
			result[index++] = static_cast<int>(IntegerValue(v)->content);
		
		if (index >= 2) break;
	}
	return result;
}

vec3 et::arrayToVec3(ArrayValue a)
{
	vec3 result;
	int index = 0;
	for (auto v : a->content)
	{
		if (v->valueClass() == ValueClass_Float)
			result[index++] = FloatValue(v)->content;
		else if (v->valueClass() == ValueClass_Integer)
			result[index++] = static_cast<float>(IntegerValue(v)->content);
		
		if (index >= 3) break;
	}
	return result;
}

vec4 et::arrayToVec4(ArrayValue a)
{
	vec4 result;
	int index = 0;
	for (auto v : a->content)
	{
		if (v->valueClass() == ValueClass_Float)
			result[index++] = FloatValue(v)->content;
		else if (v->valueClass() == ValueClass_Integer)
			result[index++] = static_cast<float>(IntegerValue(v)->content);
		
		if (index >= 4) break;
	}
	return result;
}

quaternion et::arrayToQuaternion(ArrayValue a)
{
	quaternion result;

	int index = 0;
	for (auto v : a->content)
	{
		if (v->valueClass() == ValueClass_Float)
			result[index++] = FloatValue(v)->content;
		else if (v->valueClass() == ValueClass_Integer)
			result[index++] = static_cast<float>(IntegerValue(v)->content);

		if (index >= 4) break;
	}
	return result;
}

rect et::arrayToRect(ArrayValue a)
{
	rect result;
	uint32_t index = 0;
	for (auto v : a->content)
	{
		if (v->valueClass() == ValueClass_Float)
			result[index++] = FloatValue(v)->content;
		else if (v->valueClass() == ValueClass_Integer)
			result[index++] = static_cast<float>(IntegerValue(v)->content);
		
		if (index >= 4) break;
	}
	return result;
}
