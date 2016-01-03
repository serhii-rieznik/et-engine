/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et/platform/platformtools.h>

#if (ET_PLATFORM_IOS)

#import <objc/runtime.h>
#import <UIKit/UIDevice.h>
#import <UIKit/UIView.h>
#import <UIKit/UIWindow.h>
#import <UIKit/UIViewController.h>
#import <UIKit/UIDocumentInteractionController.h>
#import <AssetsLibrary/AssetsLibrary.h>

#include <functional>
#include <sys/xattr.h>
#include <sys/utsname.h>
#include <et/platform-ios/ios.h>

@interface InteractorControllerHandler : NSObject <UIDocumentInteractionControllerDelegate>

+ (instancetype)sharedInteractorControllerHandler;

- (void)presentDocumentController:(NSNumber*)option;

@end

using namespace et;

static UIDocumentInteractionController* sharedInteractionController = nil;

void et::ios::setIdleTimerEnabled(bool enabled)
{
	[[UIApplication sharedApplication] setIdleTimerDisabled:enabled ? YES : NO];
}

void et::ios::excludeFileFromICloudBackup(const std::string& path)
{
	NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
	
	float version = [[[UIDevice currentDevice] systemVersion] floatValue];
	if (version >= 5.1f)
	{
		NSError* error = nil;
		
		[url setResourceValue:[NSNumber numberWithBool:YES]
			forKey:NSURLIsExcludedFromBackupKey error:&error];
			
		if (error != nil)
			NSLog(@"Failed to exclude file from iCloud backup: %s, error %@", path.c_str(), error);
	}
	else
	{
		u_int8_t attrValue = 1;
		int result = setxattr(path.c_str(), "com.apple.MobileBackup",
			&attrValue, sizeof(attrValue), 0, 0);
		
		if (result != 0)
			NSLog(@"Failed to exclude file from iCloud backup: %s", path.c_str());
	}
}

void et::ios::shareFile(const std::string& path, const std::string& scheme, Dictionary options, bool displayOptions)
{
	if (sharedInteractionController == nil)
	{
		sharedInteractionController = [[UIDocumentInteractionController alloc] init];
		sharedInteractionController.delegate = [InteractorControllerHandler sharedInteractorControllerHandler];
	}
	sharedInteractionController.UTI = [NSString stringWithUTF8String:scheme.c_str()];
	sharedInteractionController.URL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
	
	NSMutableDictionary* annotation = [NSMutableDictionary dictionary];
	for (const auto& kv : options->content)
	{
		if (kv.second->valueClass() == ValueClass_String)
		{
			StringValue sValue(kv.second);
			[annotation setObject:[NSString stringWithUTF8String:sValue->content.c_str()]
				forKey:[NSString stringWithUTF8String:kv.first.c_str()]];
		}
		else if (kv.second->valueClass() == ValueClass_Integer)
		{
			IntegerValue iValue(kv.second);
			[annotation setObject:[NSNumber numberWithLongLong:iValue->content]
				forKey:[NSString stringWithUTF8String:kv.first.c_str()]];
		}
		else if (kv.second->valueClass() == ValueClass_Float)
		{
			IntegerValue fValue(kv.second);
			[annotation setObject:[NSNumber numberWithFloat:fValue->content]
				forKey:[NSString stringWithUTF8String:kv.first.c_str()]];
		}
	}
	sharedInteractionController.annotation = annotation;
	
	[[InteractorControllerHandler sharedInteractorControllerHandler]
		performSelectorOnMainThread:@selector(presentDocumentController:)
		withObject:[NSNumber numberWithInt:displayOptions] waitUntilDone:NO];
}

std::string et::selectFile(const StringList&, SelectFileMode, const std::string&)
{
	return emptyString;
}

bool et::ios::canOpenURL(const std::string& s)
{
	return [[UIApplication sharedApplication]
		canOpenURL:[NSURL URLWithString:[NSString stringWithUTF8String:s.c_str()]]];
}

void et::ios::saveImageToPhotos(const std::string& path, std::function<void(bool)> callback)
{
	ALAssetsLibrary* library = [[ALAssetsLibrary alloc] init];
	
	[library writeImageDataToSavedPhotosAlbum:[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]]
		metadata:nil completionBlock:^(NSURL*, NSError* error)
	{
		callback(error == nil);
		
		if (error != nil)
			NSLog(@"Unable to save image %s to Saved Photos Album:\n%@", path.c_str(), error);
	}];
	
#if (!ET_OBJC_ARC_ENABLED)
	[library release];
#endif
}

std::string et::ios::hardwareIdentifier()
{
	struct utsname name = { };
	uname(&name);
	return lowercase(std::string(name.machine));
}

std::string et::ios::systemVersion()
{
	return std::string([[[UIDevice currentDevice] systemVersion] UTF8String]);
}

bool et::ios::runningOnIPad()
{
	return UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad;
}

void et::alert(const std::string& title, const std::string& message, const std::string& button, AlertType)
{
	NSString* nsTitle = [NSString stringWithCString:title.c_str() encoding:NSUTF8StringEncoding];
	NSString* nsMessage = [NSString stringWithCString:message.c_str() encoding:NSUTF8StringEncoding];
	NSString* nsButton = [NSString stringWithCString:button.c_str() encoding:NSUTF8StringEncoding];
	
	UIAlertView* alert = [[UIAlertView alloc] initWithTitle:nsTitle message:nsMessage delegate:nil
		cancelButtonTitle:nsButton otherButtonTitles:nil];
	
	[alert show];
	
#if (!ET_OBJC_ARC_ENABLED)
	[alert release];
#endif
}


/*
 * Obj-C stuff
 */
@implementation InteractorControllerHandler

+ (instancetype)sharedInteractorControllerHandler
{
	static InteractorControllerHandler* sharedInstance = nil;
	static dispatch_once_t onceToken = 0;
	dispatch_once(&onceToken, ^{
		sharedInstance = [[InteractorControllerHandler alloc] init];
	});
	return sharedInstance;
}

- (void)presentDocumentController:(NSNumber*)option
{
	UIViewController* handle = (__bridge UIViewController*)((void*)application().renderingContextHandle());
	
	CGRect presentRect = handle.view.bounds;
	presentRect.origin.y = presentRect.size.height - 1.0f;
	presentRect.size.height = 1.0f;
	
	ApplicationNotifier().notifyDeactivated();
	
	if ([option boolValue])
		[sharedInteractionController presentOptionsMenuFromRect:presentRect inView:handle.view animated:YES];
	else
		[sharedInteractionController presentOpenInMenuFromRect:presentRect inView:handle.view animated:YES];
}

- (void)documentInteractionControllerWillPresentOpenInMenu:(UIDocumentInteractionController*)controller
{
	(void)controller;
	ApplicationNotifier().notifyDeactivated();
}

- (void)documentInteractionControllerDidDismissOpenInMenu:(UIDocumentInteractionController*)controller
{
	(void)controller;
	ApplicationNotifier().notifyActivated();
}

- (void)documentInteractionControllerWillPresentOptionsMenu:(UIDocumentInteractionController*)controller
{
	(void)controller;
	ApplicationNotifier().notifyDeactivated();
}

- (void)documentInteractionControllerDidDismissOptionsMenu:(UIDocumentInteractionController*)controller
{
	(void)controller;
	ApplicationNotifier().notifyActivated();
}

@end

#endif // ET_PLATFORM_IOS
