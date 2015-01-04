/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

#define PI								3.1415926535897932384626433832795f
#define DOUBLE_PI						6.283185307179586476925286766559f
#define HALF_PI							1.5707963267948966192313216916398f
#define QUARTER_PI						0.78539816339744830961566084581988f

#define DEG_1							0.01745329251994329576923690768489f
#define DEG_15							0.26179938779914943653855361527329f
#define DEG_30							0.52359877559829887307710723054658f
#define DEG_45							0.78539816339744830961566084581988f
#define DEG_60							1.0471975511965977461542144610932f
#define DEG_90							1.5707963267948966192313216916398f

#define TO_DEGREES						57.295779513082320876798154814105f
#define TO_RADIANS						0.01745329251994329576923690768489f

#define SQRT_2							1.4142135623730950488016887242097f
#define INV_SQRT_2						0.70710678118654752440084436210485f

#define SQRT_3							1.732050807568877f
#define INV_SQRT_3						0.577350269189626f

#define LN_2							0.693147180559945f
#define INV_LN_2						1.44269504088896f

#define GOLDEN_RATIO					1.618033988749895f

#define HORSEPOWER_TO_WATT				735.49875f

#define ET_BACKSPACE					8
#define ET_TAB							9
#define ET_NEWLINE						10
#define ET_RETURN						13
#define ET_SPACE						32

#if (ET_PLATFORM_MAC)
#
#	define ET_KEY_RETURN				36
#	define ET_KEY_TAB					48
#	define ET_KEY_SPACE					49
#	define ET_KEY_ESCAPE				53
#	define ET_KEY_BACKSPACE				51
#	define ET_KEY_SHIFT					56
#
#	define ET_KEY_LEFT					123
#	define ET_KEY_RIGHT					124
#	define ET_KEY_DOWN					125
#	define ET_KEY_UP					126
#
#	define ET_KEY_W						13
#	define ET_KEY_A						0
#	define ET_KEY_S						1
#	define ET_KEY_D						2
#
#elif (ET_PLATFORM_IOS)
#
#	define ET_KEY_RETURN				1
#	define ET_KEY_TAB					2
#	define ET_KEY_SPACE					3
#	define ET_KEY_ESCAPE				4
#	define ET_KEY_BACKSPACE				5
#
#	define ET_KEY_LEFT					6
#	define ET_KEY_RIGHT					7
#	define ET_KEY_DOWN					8
#	define ET_KEY_UP					9
#
#elif (ET_PLATFORM_WIN)
#
#	define ET_KEY_RETURN				VK_RETURN
#	define ET_KEY_TAB					VK_TAB
#	define ET_KEY_SPACE					VK_SPACE
#	define ET_KEY_ESCAPE				VK_ESCAPE
#	define ET_KEY_BACKSPACE				VK_BACK
#	define ET_KEY_SHIFT					VK_SHIFT
#
#	define ET_KEY_LEFT					VK_LEFT
#	define ET_KEY_RIGHT					VK_RIGHT
#	define ET_KEY_DOWN					VK_DOWN
#	define ET_KEY_UP					VK_UP
#
#	define ET_KEY_A						'A'	
#	define ET_KEY_S						'S'	
#	define ET_KEY_D						'D'	
#	define ET_KEY_W						'W'	
#
#elif (ET_PLATFORM_ANDROID)
#
#	define ET_KEY_RETURN				13
#	define ET_KEY_TAB					9
#	define ET_KEY_SPACE					32
#	define ET_KEY_ESCAPE				27
#	define ET_KEY_BACKSPACE				8
#
#	define ET_KEY_LEFT					123
#	define ET_KEY_RIGHT					124
#	define ET_KEY_DOWN					125
#	define ET_KEY_UP					126
#
#endif

#define ET_DEFAULT_DELIMITER			';'
#define ET_DEFAULT_DELIMITER_STRING		";"
