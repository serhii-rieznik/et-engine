/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <AppKit/NSApplication.h>
#include <AppKit/NSWindow.h>

#include <et/core/tools.h>
#include <et/input/input.h>

using namespace et;

bool Input::canGetCurrentPointerInfo()
	{ return true; }

PointerInputInfo Input::currentPointer()
{
	NSPoint location = [NSEvent mouseLocation];
	
	NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];
	NSRect frame = [keyWindow contentRectForFrameRect:[keyWindow frame]];
	location = [keyWindow convertRectFromScreen:NSMakeRect(location.x, location.y, 1.0f, 1.0f)].origin;
	
	PointerInputInfo result;
	
	result.timestamp = queryContiniousTimeInSeconds();
	result.pos = vec2(static_cast<float>(location.x), static_cast<float>(frame.size.height - location.y));
	result.normalizedPos = result.pos /
		vec2(static_cast<float>(frame.size.width), static_cast<float>(frame.size.height));
	
	return result;
}

void Input::activateSoftwareKeyboard()
{
}

void Input::deactivateSoftwareKeyboard()
{
}
