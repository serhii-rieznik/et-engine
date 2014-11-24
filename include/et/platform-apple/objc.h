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
