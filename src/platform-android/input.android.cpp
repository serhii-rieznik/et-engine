/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/input/input.h>

using namespace et;

PointerInputInfo Input::currentPointer() const
{
	return PointerInputInfo(PointerType_None, vec2(0.0f), vec2(0.0f), vec2(0.0f), 0,
		queryTime(), PointerOrigin_Touchscreen);
}

bool Input::canGetCurrentPointerInfo() const
{
	return false;
}

void Input::activateSoftwareKeyboard()
{
}

void Input::deactivateSoftwareKeyboard()
{
}
