/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

#if (ET_PLATFORM_MAC)

#include <et/platform-mac/mac.h>
#include <Foundation/NSString.h>
#include <Foundation/NSURL.h>
#include <Foundation/NSBundle.h>
#include <AppKit/NSWorkspace.h>

using namespace et;

bool et::mac::canOpenURL(const std::string& s)
{
	return [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:
		[NSURL URLWithString:[NSString stringWithUTF8String:s.c_str()]]] != nil;
}

#endif // ET_PLATFORM_MAC
