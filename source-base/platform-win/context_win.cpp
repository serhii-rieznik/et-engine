/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/platform-win/context_win.h>

namespace et
{

PlatformDependentContext ApplicationContextFactoryWin::createContextWithOptions(ContextOptions&)
{
	return PlatformDependentContext();
}

void ApplicationContextFactoryWin::destroyContext(PlatformDependentContext context)
{
}

}
