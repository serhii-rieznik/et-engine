/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#import <UIKit/UIKit.h>
#import <Social/Social.h>
#import <Twitter/Twitter.h>
#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et/platform-ios/social.h>

using namespace et;
using namespace social;

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
	NSMutableDictionary* values = [[NSMutableDictionary alloc] initWithObjectsAndKeys:SLServiceTypeTwitter, @"service", nil];
	
	if (text.size())
		[values setObject:[NSString stringWithUTF8String:text.c_str()] forKey:@"text"];
	
	if (pathToImage.size())
		[values setObject:[NSString stringWithUTF8String:pathToImage.c_str()] forKey:@"image"];
	
	if (url.size())
		[values setObject:[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]] forKey:@"url"];
	
	[[SocialController sharedSocialController] performSelectorOnMainThread:@selector(shareWithOptions:)
		withObject:values waitUntilDone:YES];
	
	[values release];
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
		withObject:values waitUntilDone:YES];
	
	[values release];
}

/*
 *
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
	SLComposeViewController* tw = [SLComposeViewController composeViewControllerForServiceType:[d objectForKey:@"service"]];
	
	if ([d objectForKey:@"text"] != nil)
		[tw setInitialText:[d objectForKey:@"text"]];
	
	if ([d objectForKey:@"image"] != nil)
		[tw addImage:[UIImage imageWithContentsOfFile:[d objectForKey:@"image"]]];
		 
	 if ([d objectForKey:@"image"] != nil)
		 [tw addURL:[d objectForKey:@"url"]];
	
	tw.completionHandler = ^(TWTweetComposeViewControllerResult result)
	{
		ApplicationNotifier().notifyActivated();
		UIViewController* vc = reinterpret_cast<UIViewController*>(et::application().renderingContextHandle());
		[vc dismissViewControllerAnimated:YES completion:nil];
	};
	
	ApplicationNotifier().notifyDeactivated();
	UIViewController* vc = reinterpret_cast<UIViewController*>(et::application().renderingContextHandle());
	[vc presentViewController:tw animated:YES completion:nil];
}


@end