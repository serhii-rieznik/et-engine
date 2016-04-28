/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#if (ET_OBJC_ARC_ENABLED)
#
#	define ET_OBJC_AUTORELEASE(VALUE)		(VALUE)
#	define ET_OBJC_RETAIN(VALUE)			(VALUE)
#	define ET_OBJC_RELEASE(VALUE)			{ VALUE = nil; }
#
#else
#
#	define ET_OBJC_AUTORELEASE(VALUE)		[VALUE autorelease]
#	define ET_OBJC_RETAIN(VALUE)			[VALUE retain]
#	define ET_OBJC_RELEASE(VALUE)			{ [VALUE release]; VALUE = nil; }
#
#endif
