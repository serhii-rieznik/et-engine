/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#
#	define ET_PLATFORM_WIN				1
#	define CurrentPlatform				Platform_Windows
#
#	if defined(_WIN64)
#		define ET_PLATFORM_WIN32		0
#		define ET_PLATFORM_WIN64		1
#	else
#		define ET_PLATFORM_WIN32		1
#		define ET_PLATFORM_WIN64		0
#	endif
#
#elif (TARGET_OS_IPHONE)
#
#	define ET_PLATFORM_IOS				1
#	define CurrentPlatform				Platform_iOS
#
#elif (TARGET_OS_MAC)
#
#	define ET_PLATFORM_MAC				1
#	define CurrentPlatform				Platform_Mac
#
#elif (__ANDROID__)
#
#	define ET_PLATFORM_ANDROID			1
#	define CurrentPlatform				Platform_Android
#
#else
#
#	error Unable to determine current platform
#
#endif

#if (ET_PLATFORM_IOS || ET_PLATFORM_MAC)
#
#	define ET_PLATFORM_APPLE	1
#
#else
#
#	define ET_PLATFORM_APPLE	0
#
#endif

#if defined(DEBUG) || defined(_DEBUG) || defined(NDK_DEBUG)
#
#	if defined(NDEBUG)
#		error DEBUG and NDEBUG can not be defined simultaneously
#	endif
#
#	define ET_DEBUG	1
#
#else
#
#	define ET_DEBUG	0
#
#endif

#define ET_TO_CONST_CHAR_IMPL(a)	#a
#define ET_TO_CONST_CHAR(a)			ET_TO_CONST_CHAR_IMPL(a)

namespace et
{
	enum Platform
	{
		Platform_Windows,
		Platform_iOS,
		Platform_Mac,
		Platform_Android
	};
	
	enum Architecture
	{
		Architecture_Unknown,
		
		Architecture_32bit = 1,
		Architecture_64bit = 2,
		
		currentArchitecture = (sizeof(void*) == 4) ? Architecture_32bit :
			((sizeof(void*) == 8) ? Architecture_64bit : Architecture_Unknown)
	};
	
}
