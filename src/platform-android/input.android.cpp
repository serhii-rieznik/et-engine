/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/input/input.h>

#if (ET_PLATFORM_ANDROID)

using namespace et;

namespace et
{
	float queryContiniousTimeInSeconds();
}

PointerInputInfo Input::currentPointer()
{
	return PointerInputInfo(PointerType_None, vec2(0.0f), vec2(0.0f), vec2(0.0f), 0,
		queryContiniousTimeInSeconds(), PointerOrigin_Touchscreen);
}

bool Input::canGetCurrentPointerInfo()
{
	return false;
}

void Input::activateSoftwareKeyboard()
{
}

void Input::deactivateSoftwareKeyboard()
{
}

#endif // ET_PLATFORM_ANDROID
