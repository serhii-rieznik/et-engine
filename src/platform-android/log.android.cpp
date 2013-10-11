/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/platform-android/nativeactivity.h>
#include <et/core/et.h>

#define JUST_DO_IT(LEVEL)	va_list args;\
							va_start(args, format);\
							__android_log_vprint(LEVEL, "et", format, args);\
							va_end(args);


void et::log::debug(const char* format, ...)
{
#if (ET_DEBUG)
	JUST_DO_IT(ANDROID_LOG_DEBUG)
#endif
}

void et::log::info(const char* format, ...)
{
	JUST_DO_IT(ANDROID_LOG_INFO)
}

void et::log::warning(const char* format, ...)
{
	JUST_DO_IT(ANDROID_LOG_WARN)
}

void et::log::error(const char* format, ...)
{
	JUST_DO_IT(ANDROID_LOG_ERROR)
}

void et::log::debug(const wchar_t* format, ...)
{
}

void et::log::info(const wchar_t* format, ...)
{
}

void et::log::warning(const wchar_t* format, ...)
{
}

void et::log::error(const wchar_t* format, ...)
{
}