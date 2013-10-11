/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#import <UIKit/UIKit.h>
#import <Twitter/Twitter.h>
#include <et/app/application.h>
#include <et/platform-ios/twitter.h>

using namespace et;

bool Twitter::canTweet()
{
	return [TWTweetComposeViewController canSendTweet] ? true : false;
}

void Twitter::tweet(const std::string& text, const std::string& pathToImage, const std::string& url)
{
	TWTweetComposeViewController* tw = [[TWTweetComposeViewController alloc] init];
	
	if (text.size())
		[tw setInitialText:[NSString stringWithUTF8String:text.c_str()]];
	
	if (pathToImage.size())
		[tw addImage:[UIImage imageWithContentsOfFile:[NSString stringWithUTF8String:pathToImage.c_str()]]];
	
	if (url.size())
		[tw addURL:[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]]];
	
	tw.completionHandler = ^(TWTweetComposeViewControllerResult result)
	{
		UIViewController* vc = reinterpret_cast<UIViewController*>(et::application().renderingContextHandle());
		[vc dismissModalViewControllerAnimated:YES];
	};
	
	UIViewController* vc = reinterpret_cast<UIViewController*>(et::application().renderingContextHandle());
	[vc presentModalViewController:tw animated:YES];
	[tw autorelease];
}