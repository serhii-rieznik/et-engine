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

std::string Locale::time()
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

std::string Locale::date()
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

size_t Locale::currentLocale()
{
    NSString* localeId = [[NSLocale preferredLanguages] objectAtIndex:0];
	std::string mbcs = [localeId cStringUsingEncoding:NSUTF8StringEncoding];
	lowercase(mbcs);
	
	int32_t result = 0;

	if (mbcs.size() > 0)
		result |= mbcs[0];
	
	if (mbcs.size() > 1)
		result |= mbcs[1] << 8;
	
	if ((mbcs.size() >= 5) && ((mbcs[2] == '-') || (mbcs[2] == '_')))
		result |= (mbcs[3] << 16) | (mbcs[4] << 24);
	else
		result |= (result & 0xffff) << 16;

	return static_cast<size_t>(result);
}
