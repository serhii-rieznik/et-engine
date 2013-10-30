/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <Foundation/NSDateFormatter.h>
#include <et/core/tools.h>
#include <et/core/containers.h>
#include <et/locale/locale.h>

using namespace et;

std::string locale::time()
{
	NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
	
#if (!ET_OBJC_ARC_ENABLED)
	[formatter autorelease];
#endif
	
	[formatter setTimeStyle:NSDateFormatterMediumStyle];
	[formatter setDateStyle:NSDateFormatterNoStyle];
	[formatter setLocale:[NSLocale currentLocale]];
	return std::string([[formatter stringFromDate:[NSDate date]] cStringUsingEncoding:NSUTF8StringEncoding]);
}

std::string locale::date()
{
	NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
	
#if (!ET_OBJC_ARC_ENABLED)
	[formatter autorelease];
#endif
	
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
