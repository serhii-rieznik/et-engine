/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>

#if (ET_PLATFORM_IOS)

#import <UIKit/UIKit.h>
#import <Social/Social.h>
#include <et/platform-apple/objc.h>
#include <et/platform-ios/social.h>

namespace et
{
namespace social
{

Event1<bool> et::social::notifications::sharingFinished = Event1<bool>();

@interface SocialController : NSObject

+ (instancetype)sharedSocialController;

- (void)shareWithOptions:(NSDictionary*)d;

@end

bool et::social::canTweet()
{
	return [SLComposeViewController isAvailableForServiceType:SLServiceTypeTwitter] ? true : false;
}

bool et::social::canPostToFacebook()
{
	return [SLComposeViewController isAvailableForServiceType:SLServiceTypeFacebook] ? true : false;
}

void et::social::tweet(const std::string& text, const std::string& pathToImage, const std::string& url)
{
	__block NSMutableDictionary* values = [[NSMutableDictionary alloc] initWithObjectsAndKeys:SLServiceTypeTwitter, @"service", nil];
	
	if (!text.empty())
		[values setObject:[NSString stringWithUTF8String:text.c_str()] forKey:@"text"];
	
	if (!pathToImage.empty())
		[values setObject:[NSString stringWithUTF8String:pathToImage.c_str()] forKey:@"image"];
	
	if (!url.empty())
		[values setObject:[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]] forKey:@"url"];
	
	dispatch_async(dispatch_get_main_queue(), ^
	{
		[[SocialController sharedSocialController] shareWithOptions:values];
		ET_OBJC_RELEASE(values)
	});
}

void et::social::postToFacebook(const std::string& text, const std::string& pathToImage, const std::string& url)
{
	NSMutableDictionary* values = [[NSMutableDictionary alloc] initWithObjectsAndKeys:SLServiceTypeFacebook, @"service", nil];
	
	if (text.size())
		[values setObject:[NSString stringWithUTF8String:text.c_str()] forKey:@"text"];
	
	if (pathToImage.size())
		[values setObject:[NSString stringWithUTF8String:pathToImage.c_str()] forKey:@"image"];
	
	if (url.size())
		[values setObject:[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]] forKey:@"url"];
	
	[[SocialController sharedSocialController] performSelectorOnMainThread:@selector(shareWithOptions:)
		withObject:values waitUntilDone:NO];
	
	ET_OBJC_RELEASE(values)
}

/*
 * Social Controller
 */
@implementation SocialController

+ (instancetype)sharedSocialController
{
	static SocialController* _sharedSocialController = nil;
	static dispatch_once_t onceToken = 0;
	dispatch_once(&onceToken, ^{
		_sharedSocialController = [[SocialController alloc] init];
	});
	return _sharedSocialController;
}

- (void)shareWithOptions:(NSDictionary*)d
{
	SLComposeViewController* composer =
		[SLComposeViewController composeViewControllerForServiceType:[d objectForKey:@"service"]];
	
	if ([d objectForKey:@"text"] != nil)
		[composer setInitialText:[d objectForKey:@"text"]];
	
	if ([d objectForKey:@"image"] != nil)
		[composer addImage:[UIImage imageWithContentsOfFile:[d objectForKey:@"image"]]];
		 
	 if ([d objectForKey:@"url"] != nil)
		 [composer addURL:[d objectForKey:@"url"]];
	
	composer.completionHandler = ^(SLComposeViewControllerResult result)
	{
		notifications::sharingFinished.invokeInMainRunLoop(result == SLComposeViewControllerResultDone);
		UIViewController* vc = (__bridge UIViewController*)
			reinterpret_cast<void*>(et::application().renderingContextHandle());
		[vc dismissViewControllerAnimated:YES completion:nil];
	};
	
	UIViewController* vc = (__bridge UIViewController*)
		reinterpret_cast<void*>(et::application().renderingContextHandle());
	[vc presentViewController:composer animated:YES completion:nil];
}


@end

}
}

#endif // ET_PLATFORM_IOS
