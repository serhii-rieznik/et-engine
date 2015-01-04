/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

namespace et
{
	std::string unicodeToUtf8(const std::wstring& w);
	std::wstring utf8ToUnicode(const std::string& mbcs);
		
	inline std::string intToStr(int value)
	{
		char buffer[32] = { };
		sprintf(buffer, "%d", value);
		return buffer;
	}

	inline std::string intToStr(unsigned long value)
	{
		char buffer[32] = { };
		sprintf(buffer, "%lu", value);
		return buffer;
	}
	
	inline std::string intToStr(unsigned int value)
	{
		char buffer[32] = { };
		sprintf(buffer, "%u", value);
		return buffer;
	}

	inline std::string intToStr(int64_t value)
	{
		char buffer[64] = { };
		sprintf(buffer, "%lld", value);
		return buffer;
	}
	
	inline std::string intToStr(uint64_t value)
	{
		char buffer[64] = { };
		sprintf(buffer, "%llu", value);
		return buffer;
	}

	inline std::string intToStr(void* value)
	{
		char buffer[32] = { };
		sprintf(buffer, "%08llX", reinterpret_cast<uint64_t>(value));
		return buffer;
	}

	inline std::string intToStr(const void* value)
	{
		char buffer[32] = { };
		sprintf(buffer, "%08llX", reinterpret_cast<uint64_t>(value));
		return buffer;
	}

	inline int strToInt(const std::string& value)
		{ return std::atoi(value.c_str()); }

	inline long strToLong(const std::string& value)
		{ return std::atol(value.c_str()); }

	inline long long strToLongLong(const std::string& value)
		{ return std::atoll(value.c_str()); }
	
	inline double strToDouble(const std::string& value)
		{ return std::atof(value.c_str()); }

	inline float strToFloat(const std::string& value)
		{ return static_cast<float>(strToDouble(value)); }

	inline float strToFloat(const std::wstring& value)
		{ return std::wcstof(value.c_str(), nullptr); }
	
	inline bool strToBool(std::string s)
	{
		for (auto& c : s)
			c = static_cast<char>(::tolower(c) & 0xff);
		return (s == "true") || (s == "1");
	}

	int hexCharacterToInt(int c);

	std::string floatToStr(float value, int precission = 5);
	std::string floatToTimeStr(float value, bool showMSec = true);
}
