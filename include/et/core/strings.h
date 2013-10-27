/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

#if (ET_SUPPORT_INITIALIZER_LIST)
#
#	define ET_STRINGLIST(...)		StringList({ __VA_ARGS__ })
#
#else
#
#	define ET_STRINGLIST(...)		([&]()->StringList{const std::string et_sl[]={__VA_ARGS__};\
										return StringList(et_sl, et_sl+sizeof(et_sl)/sizeof(std::string));})()
#
#
#endif

namespace et
{
	typedef std::vector<std::string> StringList;
	
	inline void lowercase(std::string& s)
		{ for (auto& c : s) c = static_cast<char>(::tolower(c) & 0xff); }
	
	inline void uppercase(std::string& s)
		{ for (auto& c : s) c = static_cast<char>(::toupper(c) & 0xff); }
	
	inline bool isNewLineChar(char c)
		{ return (c == ET_RETURN) || (c == ET_NEWLINE); }
	
	inline bool isNewLineChar(wchar_t c)
		{ return (c == ET_RETURN) || (c == ET_NEWLINE); }
	
	inline bool isWhitespaceChar(char c)
		{ return (c == ET_SPACE) || (c == ET_RETURN) || (c == ET_NEWLINE) || (c == ET_TAB); }
	
	inline bool isWhitespaceChar(wchar_t c)
		{ return (c == ET_SPACE) || (c == ET_RETURN) || (c == ET_NEWLINE) || (c == ET_TAB); }
	
	inline std::string lowercase(const std::string& s)
	{
		std::string str(s);
		lowercase(str);
		return str;
	}
	
	inline std::string uppercase(const std::string& s)
	{
		std::string str(s);
		uppercase(str);
		return str;
	}
	
	float extractFloat(std::string& s);
	
	std::string capitalize(std::string);
	
	std::string& trim(std::string &str);
	std::string removeWhitespace(const std::string&);
	StringList split(const std::string& s, const std::string& delim);
}
