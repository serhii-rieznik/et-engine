/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <Foundation/NSArray.h>
#include <Foundation/NSDate.h>
#include <Foundation/NSDateFormatter.h>
#include <Foundation/NSLocale.h>
#include <Foundation/NSString.h>
#include <et/core/tools.h>
#include <et/core/containers.h>
#include <et/locale/locale.h>
#include <et/platform-apple/objc.h>

using namespace et;

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

size_t locale::currentLocale()
{
    NSString* localeId = [[NSLocale preferredLanguages] objectAtIndex:0];
	return localeToIdentifier([localeId cStringUsingEncoding:NSUTF8StringEncoding]);
}
