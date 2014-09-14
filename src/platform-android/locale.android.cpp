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

#if (ET_PLATFORM_ANDROID)

using namespace et;

std::string locale::time()
{
	return std::string();
}

std::string locale::date()
{
	return std::string();
}

std::string locale::currentLocale()
{
	return "en-US";
}

#endif // ET_PLATFORM_ANDROID
