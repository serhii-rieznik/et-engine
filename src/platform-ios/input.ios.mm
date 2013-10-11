/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <Foundation/NSNotification.h>
#include <et/core/tools.h>
#include <et/input/input.h>

using namespace et;

extern NSString* etKeyboardRequiredNotification;
extern NSString* etKeyboardNotRequiredNotification;

bool Input::canGetCurrentPointerInfo()
	{ return false; }

PointerInputInfo Input::currentPointer()
{
	return PointerInputInfo(PointerType_None, vec2(0.0f), vec2(0.0f),
		vec2(0.0f), 0, queryContiniousTimeInSeconds(), PointerOrigin_Touchscreen);
}

void Input::activateSoftwareKeyboard()
{
	[[NSNotificationCenter defaultCenter]
		postNotificationName:etKeyboardRequiredNotification object:nil];
}

void Input::deactivateSoftwareKeyboard()
{
	[[NSNotificationCenter defaultCenter]
		postNotificationName:etKeyboardNotRequiredNotification object:nil];
}
