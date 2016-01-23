/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <signal.h>

namespace et
{
	namespace debug
	{
		void debugBreak()
		{
			__asm int 3; // __builtin_trap();
		}
	}
}
