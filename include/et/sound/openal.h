/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

#if (ET_PLATFORM_MAC || ET_PLATFORM_IOS)
#	include <OpenAL/al.h>
#	include <OpenAL/alc.h>
#elif (ET_PLATFORM_ANDROID)
#	include <oal/al.h>
#	include <oal/alc.h>
#elif (ET_PLATFORM_WIN)
#	include <oal\al.h>
#	include <oal\alc.h>
#else
#	error Unknown platform
#endif

#if (ET_DEBUG)
#   define checkOpenALError(...) checkOpenALErrorEx(ET_CALL_FUNCTION, __FILE__, ET_TO_CONST_CHAR(__LINE__), __VA_ARGS__)
#else
#   define checkOpenALError(...)
#endif

namespace et
{
	namespace audio
	{
		size_t openALFormatFromChannelsAndBitDepth(size_t numChannels, size_t bitDepth);
		
		void checkOpenALErrorEx(const char* caller, const char* file, const char* line, const char* tag, ...);
	}
}
