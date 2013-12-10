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

#if (ET_DEBUG)
#	define setDebugVariable(var, val) et::et_internal_setDebugVariable(var, val, #var, #val, ET_CALL_FUNCTION);
#else
#	define setDebugVariable(var, val) var = val;
#endif

#if (ET_PLATFORM_WIN)
#
#	define ET_CALL_FUNCTION					__FUNCTION__
#
#	define _CRTDBG_MAP_ALLOC
#	include <crtdbg.h>
#	include <stdarg.h>
#
#	define ET_SUPPORT_RANGE_BASED_FOR		(_MSC_FULL_VER >= 170060315)
#	define ET_SUPPORT_INITIALIZER_LIST		(_MSC_FULL_VER >= 180020617)
#	define ET_SUPPORT_VARIADIC_TEMPLATES	(_MSC_FULL_VER >= 180020617)
#
#	define ET_DEPRECATED					__declspec(deprecated)
#	define ET_FORMAT_FUNCTION
#
#elif (ET_PLATFORM_APPLE)
#
#	define ET_CALL_FUNCTION					__PRETTY_FUNCTION__
#
#	define ET_SUPPORT_RANGE_BASED_FOR		(__has_feature(cxx_range_for) || __has_extension(cxx_range_for))
#	define ET_SUPPORT_INITIALIZER_LIST		(__has_feature(cxx_generalized_initializers) || __has_extension(cxx_generalized_initializers))
#	define ET_SUPPORT_VARIADIC_TEMPLATES	(__has_feature(cxx_variadic_templates) || __has_extension(cxx_variadic_templates))
#
#	define ET_OBJC_ARC_ENABLED				(__has_feature(objc_arc))
#
#	if (ET_PLATFORM_MAC)
#		define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#	endif
#
#	define ET_DEPRECATED					__attribute__((deprecated))
#	define ET_FORMAT_FUNCTION				__attribute__((format(printf, 1, 2)))
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
#
#else
#
#	error Platform is not defined
#
#endif

#if !defined(ET_SUPPORT_RANGE_BASED_FOR)
#
#	error Please define if compiler supports range based for
#
#endif

#if (ET_SUPPORT_RANGE_BASED_FOR)
#
#	define ET_START_ITERATION(container, type, variable)	for (type variable : container) {
#
#	define ET_END_ITERATION									}
#
#else 
#
#	define ET_START_ITERATION(container, type, variable)	for (auto variable##I = container.begin(), variable##E = container.end();\
																variable##I != variable##E; ++variable##I) { type variable = *variable##I;
#
#	define ET_END_ITERATION									}
#
#endif


#define ET_ITERATE(container, type, variable, expression)	ET_START_ITERATION(container, type, variable)\
																expression;\
															ET_END_ITERATION

#define ET_LOG_MEMORY_OPERATIONS							0

#define ET_DENY_COPY(t)										private:\
																t(const t&) { }\
																t(t&&) { }\
																t& operator = (const t&) { return *this; }
																	
#define ET_COMPOSE_UINT32(A, B, C, D)					(D | (C << 8) | (B << 16) | (A << 24))
#define ET_COMPOSE_UINT32_INVERTED(A, B, C, D)			(A | (B << 8) | (C << 16) | (D << 24))

#if (ET_DEBUG)
#
#	define ET_ASSERT(C)	\
	{ \
		if ((C) == false) \
		{ \
			et::log::warning("Condition: "#C"\nfailed at: %s [%d]", __FILE__, __LINE__); \
			abort(); \
		} \
	}
#	define ET_FAIL(MSG) { log::error(#MSG"\nfailed at: %s [%d]", __FILE__, __LINE__); }
#
#else
#
#	define ET_ASSERT(C)
#	define ET_FAIL(MSG)
#
#endif

namespace et
{
	template <typename T>
	inline void et_internal_setDebugVariable(T& variable, const T& newValue,
		const char* variableName, const char* valueName, const char* function)
	{
		variable = newValue;
		std::cout << variableName << " = " << valueName << ", call from " << function << std::endl;
	}
	
#if (ET_DEBUG && ET_LOG_MEMORY_OPERATIONS)
	inline void etCopyMemory(void* dest, const void* source, size_t size)
	{
		static size_t totalMemoryCopied = 0;

		if (totalMemoryCopied > 0xffffffff - size)
			totalMemoryCopied = 0;

		totalMemoryCopied += size;
		std::cout << "[etCopyMemory] copying " << size << " bytes (" << size / 1024 << "Kb, "
			<< size / 1024 / 1024 << "Mb). Copied so far:" << totalMemoryCopied << " bytes, "
			<< totalMemoryCopied / 1024 << "Kb, " << totalMemoryCopied / 1024 / 1024 << "Mb." << std::endl;
		memcpy(dest, source, size);
	}
#else
#	define etCopyMemory memcpy
#endif

#if (ET_DEBUG && ET_LOG_MEMORY_OPERATIONS)
	inline void etFillMemory(void* dest, int value, size_t size)
	{
		static size_t totalMemoryFilled = 0;
		if (totalMemoryFilled > 0xffffffff - size)
			totalMemoryFilled = 0;
		totalMemoryFilled += size;
		std::cout << "[etFillMemory] filling " << size << " bytes (" << size / 1024 << "Kb, "
			<< size / 1024 / 1024 << "Mb). Filled so far:" << totalMemoryFilled << " bytes, "
			<< totalMemoryFilled / 1024 << "Kb, " << totalMemoryFilled / 1024 / 1024 << "Mb." << std::endl;
		memset(dest, value, size);
	}
#else
#	define etFillMemory memset
#endif
}
