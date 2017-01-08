/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#if (ET_PLATFORM_WIN)
#   include <Windows.h>
#elif (ET_PLATFORM_MAC)
#	include <signal.h>
#endif

namespace et
{
namespace debug
{

void debugBreak()
{
#if (ET_PLATFORM_WIN)
	::DebugBreak();
#elif (ET_PLATFORM_MAC)
	__asm { int 3 };
#else
#	error Define breakpoint for current platform
#endif
}

}
}
