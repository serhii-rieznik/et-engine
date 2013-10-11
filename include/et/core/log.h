/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	namespace log
	{
		void debug(const char*, ...) ET_FORMAT_FUNCTION;
		void info(const char*, ...) ET_FORMAT_FUNCTION;
		void warning(const char*, ...) ET_FORMAT_FUNCTION;
		void error(const char*, ...) ET_FORMAT_FUNCTION;

		void debug(const wchar_t*, ...);
		void info(const wchar_t*, ...);
		void warning(const wchar_t*, ...);
		void error(const wchar_t*, ...);
	}
}