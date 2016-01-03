/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

#if (ET_PLATFORM_WIN)

#include <Windows.h>

namespace et
{
	namespace debug
	{
		void debugBreak()
		{
			::DebugBreak();
		}
	}
}

#endif