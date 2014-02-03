/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <Foundation/NSString.h>
#include <Foundation/NSURL.h>
#include <AppKit/NSWorkspace.h>
#include <et/platform-mac/mac.h>

using namespace et;

bool et::mac::canOpenURL(const std::string& s)
{
	return [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:
		[NSURL URLWithString:[NSString stringWithUTF8String:s.c_str()]]] != nil;
}
