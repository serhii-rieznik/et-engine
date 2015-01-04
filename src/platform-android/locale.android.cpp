/*
* This file is part of `et engine`
* Copyright 2009-2015 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#if (ET_PLATFORM_ANDROID)

using namespace et;

std::string locale::time()
{
	return emptyString;
}

std::string locale::date()
{
	return emptyString;
}

std::string locale::currentLocale()
{
	return "en-US";
}

#endif // ET_PLATFORM_ANDROID
