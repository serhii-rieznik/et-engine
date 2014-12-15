/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

#if (ET_DEBUG)
#	define setDebugVariable(var, val) et::et_internal_setDebugVariable(var, val, #var, #val, ET_CALL_FUNCTION);
#else
#	define setDebugVariable(var, val) var = val;
#endif

#if (ET_PLATFORM_WIN)
#
#	define ET_CALL_FUNCTION					__FUNCTION__
#
#	if (ET_DEBUG)
#		define _CRTDBG_MAP_ALLOC
#		include <crtdbg.h>
#		include <stdarg.h>
#	endif
#
#	define ET_SUPPORT_RANGE_BASED_FOR		(_MSC_FULL_VER >= 170060315)
#	define ET_SUPPORT_INITIALIZER_LIST		(_MSC_FULL_VER >= 180020617)
#	define ET_SUPPORT_VARIADIC_TEMPLATES	(_MSC_FULL_VER >= 180020617)
#
#	define ET_FORMAT_FUNCTION
#
#	define ET_DEPRECATED					__declspec(deprecated)
#	define ET_ALIGNED(A)					__declspec(align(A))
#
#elif (ET_PLATFORM_APPLE)
#
#	define ET_CALL_FUNCTION					__PRETTY_FUNCTION__
#
#	define ET_SUPPORT_RANGE_BASED_FOR		(__has_feature(cxx_range_for) || __has_extension(cxx_range_for))
#	define ET_SUPPORT_INITIALIZER_LIST		(__has_feature(cxx_generalized_initializers) || __has_extension(cxx_generalized_initializers))
#	define ET_SUPPORT_VARIADIC_TEMPLATES	(__has_feature(cxx_variadic_templates) || __has_extension(cxx_variadic_templates))
#	define ET_OBJC_ARC_ENABLED				(__has_feature(objc_arc))
#
#	if (ET_PLATFORM_MAC)
#		define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#	endif
#
#	define ET_DEPRECATED					__attribute__((deprecated))
#	define ET_FORMAT_FUNCTION				__attribute__((format(printf, 1, 2)))
#	define ET_FORMAT_FUNCTION_IN_CLASS		__attribute__((format(printf, 2, 3)))
#	define ET_ALIGNED(A)					__attribute__((aligned(A)))
#
#elif (ET_PLATFORM_ANDROID)
#
#	define ET_CALL_FUNCTION					__PRETTY_FUNCTION__
#
#	define ET_SUPPORT_RANGE_BASED_FOR		(__has_feature(cxx_range_for) || __has_extension(cxx_range_for))
#	define ET_SUPPORT_INITIALIZER_LIST		(__has_feature(cxx_generalized_initializers) || __has_extension(cxx_generalized_initializers))
#	define ET_SUPPORT_VARIADIC_TEMPLATES	(__has_feature(cxx_variadic_templates) || __has_extension(cxx_variadic_templates))
#
#	define ET_OBJC_ARC_ENABLED				0
#
#	define ET_DEPRECATED					__attribute__((deprecated))
#	define ET_FORMAT_FUNCTION				__attribute__((format(printf, 1, 2)))
#	define ET_FORMAT_FUNCTION_IN_CLASS		__attribute__((format(printf, 2, 3)))
#	define ET_ALIGNED(A)					__attribute__((aligned(A)))
#
#else
#
#	error Platform is not defined
#
#endif

#define ET_LOG_MEMORY_OPERATIONS							0

#define ET_DENY_COPY(t)										private:\
																t(const t&) = delete;\
																t(t&&) = delete;\
																t& operator = (const t&) = delete;
																	
#define ET_COMPOSE_UINT32(A, B, C, D)					(D | (C << 8) | (B << 16) | (A << 24))
#define ET_COMPOSE_UINT32_INVERTED(A, B, C, D)			(A | (B << 8) | (C << 16) | (D << 24))
																	
namespace et
{
	namespace log
	{
		void debug(const char*, ...) ET_FORMAT_FUNCTION;
		void info(const char*, ...) ET_FORMAT_FUNCTION;
		void warning(const char*, ...) ET_FORMAT_FUNCTION;
		void error(const char*, ...) ET_FORMAT_FUNCTION;
	}
}

#if (ET_DEBUG)
#
#	define ET_ASSERT(C)	\
	{ \
		if (!(C)) \
		{ \
			et::log::warning("Condition: %s\nfailed at: %s [%d]", (#C), __FILE__, __LINE__); \
			abort(); \
		} \
	}
#
#else
#
#	define ET_ASSERT(C)				{ }
#
#endif

#define ET_FAIL(MSG)				{ log::error("%s\noccurred at: %s [%d]", (#MSG), __FILE__, __LINE__); abort(); }
#define ET_FAIL_FMT(FMT, ...)		{ log::error(FMT"\noccurred at %s [%d]", __VA_ARGS__, __FILE__, __LINE__); abort(); }

namespace et
{
	template <typename T>
	inline void et_internal_setDebugVariable(T& variable, const T& newValue,
		const char* variableName, const char* valueName, const char* function)
	{
		variable = newValue;
		printf("[setDebugVariable] %s:\n%s = %s\n", function, variableName, valueName);
	}
	
#if (ET_DEBUG && ET_LOG_MEMORY_OPERATIONS)
	inline void etCopyMemory(void* dest, const void* source, uint64_t size)
	{
		static uint64_t totalMemoryCopied = 0;
		totalMemoryCopied += size;
		
		printf("[etCopyMemory] copying %llu bytes (%llu/%llu), total copied: %llu (%llu/%llu)",
			size, size / 1024, size / 1024 / 1024, totalMemoryCopied, totalMemoryCopied / 1024, totalMemoryCopied / 1024 / 1024);
		
		memcpy(dest, source, size);
	}
#else
#
#	define etCopyMemory memcpy
#
#endif

#if (ET_DEBUG && ET_LOG_MEMORY_OPERATIONS)
	inline void etFillMemory(void* dest, int value, uint64_t size)
	{
		static uint64_t totalMemoryFilled = 0;
		totalMemoryFilled += size;
		
		printf("[etFillMemory] filling %llu bytes (%llu/%llu), total filled: %llu (%llu/%llu)",
			size, size / 1024, size / 1024 / 1024, totalMemoryCopied, totalMemoryCopied / 1024, totalMemoryCopied / 1024 / 1024);
		
		memset(dest, value, size);
	}
#else
#
#	define etFillMemory memset
#
#endif
}
