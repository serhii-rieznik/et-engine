/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#import <UIKit/UIDevice.h>
#import <UIKit/UIView.h>
#import <UIKit/UIWindow.h>
#import <UIKit/UIViewController.h>
#import <UIKit/UIDocumentInteractionController.h>
#import <AssetsLibrary/AssetsLibrary.h>

#include <functional>
#include <sys/xattr.h>
#include <sys/utsname.h>
#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et/platform/platformtools.h>
#include <et/platform-ios/ios.h>

@interface InteractorControllerHandler : NSObject <UIDocumentInteractionControllerDelegate>

+ (instancetype)sharedInteractorControllerHandler;

- (void)presentDocumentController:(NSNumber*)option;

@end


using namespace et;

static UIDocumentInteractionController* sharedInteractionController = nil;

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

void et::ios::shareFile(const std::string& path, const std::string& scheme, bool displayOptions)
{
	if (sharedInteractionController == nil)
	{
		sharedInteractionController = [[UIDocumentInteractionController alloc] init];
		sharedInteractionController.delegate = [InteractorControllerHandler sharedInteractorControllerHandler];
	}
	sharedInteractionController.UTI = [NSString stringWithUTF8String:scheme.c_str()];
	sharedInteractionController.URL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
	
	[[InteractorControllerHandler sharedInteractorControllerHandler]
		performSelectorOnMainThread:@selector(presentDocumentController:) withObject:[NSNumber numberWithInt:displayOptions] waitUntilDone:YES];
}

std::string et::selectFile(const StringList&, SelectFileMode, const std::string&)
{
	return std::string();
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
		metadata:nil completionBlock:^(NSURL *assetURL, NSError *error)
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

bool et::ios::runningOnIPad()
{
	return UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad;
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
	UIViewController* handle = (__bridge UIViewController*)(application().renderingContextHandle());
	
	CGRect presentRect = handle.view.bounds;
	presentRect.origin.y = presentRect.size.height - 1.0f;
	presentRect.size.height = 1.0f;
	
	ApplicationNotifier().notifyDeactivated();
	
	if ([option boolValue])
		[sharedInteractionController presentOptionsMenuFromRect:presentRect inView:handle.view animated:YES];
	else
		[sharedInteractionController presentOpenInMenuFromRect:presentRect inView:handle.view animated:YES];
}

- (void)documentInteractionControllerWillPresentOpenInMenu:(UIDocumentInteractionController *)controller
{
	ApplicationNotifier().notifyDeactivated();
}

- (void)documentInteractionControllerDidDismissOpenInMenu:(UIDocumentInteractionController *)controller
{
	ApplicationNotifier().notifyActivated();
}

- (void)documentInteractionControllerWillPresentOptionsMenu:(UIDocumentInteractionController *)controller
{
	ApplicationNotifier().notifyDeactivated();
}

- (void)documentInteractionControllerDidDismissOptionsMenu:(UIDocumentInteractionController *)controller
{
	ApplicationNotifier().notifyActivated();
}

@end
