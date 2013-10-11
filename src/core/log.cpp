/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/et.h>

#if (ET_PLATFORM_ANDROID)
#
#	error Please include platform-android implementation instead
#
#endif

#define JUST_DO_IT()					va_list args; va_start(args, format);\
										vprintf(format, args);\
										va_end(args);\
										printf("\n"); fflush(stdout);

#define JUST_DO_IT_W()					va_list args; va_start(args, format);\
										vwprintf(format, args);\
										va_end(args);\
										printf("\n"); fflush(stdout);

#define DO_IT_WITH_PREFIX(PREFIX)		printf(PREFIX); \
										va_list args; va_start(args, format);\
										vprintf(format, args);\
										va_end(args);\
										printf("\n"); fflush(stdout);

#define DO_IT_WITH_WPREFIX(PREFIX)		wprintf(PREFIX); \
										va_list args; va_start(args, format);\
										vwprintf(format, args);\
										va_end(args);\
										printf("\n"); fflush(stdout);


void et::log::debug(const char* format, ...)
{
#if (ET_DEBUG)
	JUST_DO_IT()
#else
	(void)format;
#endif
}

void et::log::info(const char* format, ...)
{
	JUST_DO_IT()
}

void et::log::warning(const char* format, ...)
{
	DO_IT_WITH_PREFIX("WARNING: ")
}

void et::log::error(const char* format, ...)
{
	DO_IT_WITH_PREFIX("ERROR: ")
}

void et::log::debug(const wchar_t* format, ...)
{
#if (ET_DEBUG)
	JUST_DO_IT_W()
#else
	(void)format;
#endif
}

void et::log::info(const wchar_t* format, ...)
{
	JUST_DO_IT_W()
}

void et::log::warning(const wchar_t* format, ...)
{
	DO_IT_WITH_WPREFIX(L"WARNING: ")
}

void et::log::error(const wchar_t* format, ...)
{
	DO_IT_WITH_WPREFIX(L"ERROR: ")
}

