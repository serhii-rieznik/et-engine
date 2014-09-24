/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
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
#if defined(ET_CONSOLE_APPLICATION)
	return false;
#else
	return [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:
		[NSURL URLWithString:[NSString stringWithUTF8String:s.c_str()]]] != nil;
#endif
}

std::string et::mac::bundleVersion()
{
	NSString* versionString = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
	return versionString == nil ? std::string() : std::string([versionString UTF8String]);
}

#endif // ET_PLATFORM_MAC
