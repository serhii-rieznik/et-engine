/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>

#if (ET_PLATFORM_IOS)

#include <Foundation/NSNotification.h>
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

#endif // ET_PLATFORM_IOS
