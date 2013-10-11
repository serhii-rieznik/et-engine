/*
* This file is part of `et engine`
* Copyright 2009-2012 by Sergey Reznik
* Please, do not modify contents without approval.
*
*/

#include <fstream>
#include <et/core/tools.h>
#include <et/core/containers.h>
#include <et/locale/locale.h>

using namespace et;

std::string Locale::time()
{
	return std::string();
}

std::string Locale::date()
{
	return std::string();
}

size_t Locale::currentLocale()
{
	std::string mbcs("en_US");
	lowercase(mbcs);
	
	size_t result = 0;

	if (mbcs.size() > 0)
		result |= mbcs[0];
	
	if (mbcs.size() > 1)
		result |= mbcs[1] << 8;
	
	if ((mbcs.size() >= 5) && ((mbcs[2] == '-') || (mbcs[2] == '_')))
		result |= (mbcs[3] << 16) | (mbcs[4] << 24);
	else
		result |= (result & 0xffff) << 16;

	return result;
}
