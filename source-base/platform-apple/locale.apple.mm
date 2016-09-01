/*
* This file is part of `et engine`
* Copyright 2009-2016 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <et/core/tools.h>
#include <et/core/containers.h>
#include <et/locale/locale.hpp>
#include <et/platform-apple/apple.h>

namespace et
{

std::string locale::time()
{
	NSDateFormatter* formatter = ET_OBJC_AUTORELEASE([[NSDateFormatter alloc] init]);
	[formatter setTimeStyle:NSDateFormatterMediumStyle];
	[formatter setDateStyle:NSDateFormatterNoStyle];
	[formatter setLocale:[NSLocale currentLocale]];
	return std::string([[formatter stringFromDate:[NSDate date]] cStringUsingEncoding:NSUTF8StringEncoding]);
}

std::string locale::date()
{
	NSDateFormatter* formatter = ET_OBJC_AUTORELEASE([[NSDateFormatter alloc] init]);
	[formatter setTimeStyle:NSDateFormatterNoStyle];
	[formatter setDateStyle:NSDateFormatterMediumStyle];
	[formatter setLocale:[NSLocale currentLocale]];
	return std::string([[formatter stringFromDate:[NSDate date]] cStringUsingEncoding:NSUTF8StringEncoding]);
}

std::string locale::dateTimeFromTimestamp(uint64_t interval)
{
	NSDate* date = [NSDate dateWithTimeIntervalSince1970:interval / 1000];
	
	NSDateFormatter* formatter = ET_OBJC_AUTORELEASE([[NSDateFormatter alloc] init]);
	[formatter setTimeStyle:NSDateFormatterMediumStyle];
	[formatter setDateStyle:NSDateFormatterMediumStyle];
	[formatter setLocale:[NSLocale currentLocale]];
	return std::string([[formatter stringFromDate:date] cStringUsingEncoding:NSUTF8StringEncoding]);
}

std::string locale::currentLocale()
{
    NSString* localeId = [[[[NSBundle mainBundle] preferredLocalizations] objectAtIndex:0] lowercaseString];
	return std::string([localeId cStringUsingEncoding:NSUTF8StringEncoding]);
}

}