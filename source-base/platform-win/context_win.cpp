/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/platform-win/context_win.h>

#if (ET_PLATFORM_WIN)

namespace et
{

PlatformDependentContext ApplicationContextFactoryWin::createContextWithOptions(RenderingAPI, ContextOptions&)
{
	return PlatformDependentContext();
}

void ApplicationContextFactoryWin::destroyContext(PlatformDependentContext context)
{
}

}

#endif