/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <chartboost.h>
#include "chartboost.ios.h"

using namespace et;

@interface ChartBoostProxyDelegate : NSObject <ChartboostDelegate>
{
	ApplicationNotifier _notifier;
}

- (void)show:(NSString*)locationTag;
- (void)showMoreGames;

@end

class et::ChartBoostProxyPrivate
{
public:
	ChartBoostProxyPrivate()
	{
		_chartBoostDelegate = [[ChartBoostProxyDelegate alloc] init];
	}
	
public:
	ChartBoostProxyDelegate* _chartBoostDelegate;
};

ChartBoostProxy::ChartBoostProxy()
{
	_private = new ChartBoostProxyPrivate();
}

void ChartBoostProxy::start(const std::string& identifier, const std::string& signature)
{
    Chartboost *cb = [Chartboost sharedChartboost];
	
    cb.appId = [NSString stringWithUTF8String:identifier.c_str()];
    cb.appSignature = [NSString stringWithUTF8String:signature.c_str()];
    cb.delegate = _private->_chartBoostDelegate;
    
    [cb startSession];
    [cb cacheMoreApps];
}

void ChartBoostProxy::show(const std::string& tag)
{
	[_private->_chartBoostDelegate show:[NSString stringWithUTF8String:tag.c_str()]];
}

void ChartBoostProxy::show()
{
	[_private->_chartBoostDelegate show:nil];
}

void ChartBoostProxy::showMoreGames()
{
	[_private->_chartBoostDelegate showMoreGames];
}

@implementation ChartBoostProxyDelegate

- (id)init
{
	self = [super init];
	return self;
}

- (void)show:(NSString*)locationTag
{
	if (locationTag == nil)
	{
		[[Chartboost sharedChartboost] cacheInterstitial];
		[[Chartboost sharedChartboost] showInterstitial];
	}
	else
	{
		[[Chartboost sharedChartboost] cacheInterstitial:locationTag];
		[[Chartboost sharedChartboost] showInterstitial:locationTag];
	}
}

- (void)showMoreGames
{
	[[Chartboost sharedChartboost] cacheMoreApps];
	[[Chartboost sharedChartboost] showMoreApps];
}

/*
 * Delegate methods
 */

- (BOOL)shouldRequestInterstitial:(NSString *)location
{
	NSLog(@"ChartBoostProxyDelegate shouldRequestInterstitial:%@", location);
	return YES;
}

- (BOOL)shouldRequestInterstitialsInFirstSession
{
	NSLog(@"ChartBoostProxyDelegate shouldRequestInterstitialsInFirstSession");
	return YES;
}

- (void)didFailToLoadInterstitial:(NSString *)location
{
	NSLog(@"ChartBoostProxyDelegate didFailToLoadInterstitial:%@", location);
}

- (void)didCacheInterstitial:(NSString *)location
{
	NSLog(@"ChartBoostProxyDelegate didCacheInterstitial:%@", location);
}

- (BOOL)shouldDisplayInterstitial:(NSString *)location
{
	[[Chartboost sharedChartboost] cacheInterstitial:location];
	_notifier.notifyDeactivated();
	return true;
}

- (void)didDismissInterstitial:(NSString *)location
{
	_notifier.notifyActivated();
}

- (void)didCloseInterstitial:(NSString *)location
{
	_notifier.notifyActivated();
}

/*
 * More apps
 */

- (BOOL)shouldDisplayMoreApps
{
	[[Chartboost sharedChartboost] cacheMoreApps];
	_notifier.notifyDeactivated();
	return YES;
}

- (void)didCacheMoreApps
{
	NSLog(@"ChartBoostProxyDelegate didCacheMoreApps");
}

- (void)didFailToLoadMoreApps
{
	NSLog(@"CChartBoostProxyDelegate didFailToLoadMoreApps");
}

- (void)didDismissMoreApps
{
	_notifier.notifyActivated();
}

- (void)didCloseMoreApps
{
	_notifier.notifyActivated();
}

- (void)didClickMoreApps
{
	_notifier.notifyActivated();
}

@end