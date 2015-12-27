/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

namespace et
{
	namespace debug
	{
		void debugBreak()
		{
			__builtin_trap();
		}
	}
}
