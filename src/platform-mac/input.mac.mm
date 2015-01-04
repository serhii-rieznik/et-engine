/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/input/input.h>

#if (ET_PLATFORM_MAC)

#include <AppKit/NSApplication.h>
#include <AppKit/NSWindow.h>
#include <AppKit/NSView.h>

using namespace et;

bool Input::canGetCurrentPointerInfo()
{
#if defined(ET_CONSOLE_APPLICATION)
	return false;
#else
	return true;
#endif
}

PointerInputInfo Input::currentPointer()
{
	PointerInputInfo result;
	
#if !defined(ET_CONSOLE_APPLICATION)
	NSPoint location = [NSEvent mouseLocation];
	NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];
	
	NSRect frame = [keyWindow.contentView convertRectToBacking:
		[keyWindow contentRectForFrameRect:[keyWindow frame]]];
	
	location = [keyWindow.contentView convertPointToBacking:
		[keyWindow convertRectFromScreen:NSMakeRect(location.x, location.y, 1.0f, 1.0f)].origin];
	
	result.timestamp = queryContiniousTimeInSeconds();
	
	result.pos = vec2(static_cast<float>(location.x),
		static_cast<float>(frame.size.height - location.y));
	
	result.normalizedPos = result.pos /
		vec2(static_cast<float>(frame.size.width), static_cast<float>(frame.size.height));
#endif
	
	return result;
}

void Input::activateSoftwareKeyboard()
{
}

void Input::deactivateSoftwareKeyboard()
{
}

#endif // ET_PLATFORM_MAC
