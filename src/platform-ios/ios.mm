/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#import <AssetsLibrary/AssetsLibrary.h>

#import <UIKit/UIDevice.h>
#import <UIKit/UIView.h>
#import <UIKit/UIViewController.h>
#import <UIKit/UIDocumentInteractionController.h>

#include <sys/xattr.h>
#include <et/app/application.h>
#include <et/platform/platformtools.h>
#include <et/platform-ios/ios.h>

using namespace et;

static UIDocumentInteractionController* sharedInteractionController = nil;

void et::excludeFileFromICloudBackup(const std::string& path)
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

void et::saveImageToPhotos(const std::string& path, void(*callback)(bool))
{
	ALAssetsLibrary* library = [[ALAssetsLibrary alloc] init];
	
	[library writeImageDataToSavedPhotosAlbum:[NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]]
		metadata:nil completionBlock:^(NSURL *assetURL, NSError *error)
	{
		callback(error == nil);
		if (error != nil)
		{
			NSLog(@"Unable to save image %s to Saved Photos Album:\n%@", path.c_str(), error);
		}
	}];
	
#if (!ET_OBJC_ARC_ENABLED)
	[library release];
#endif
}

void et::shareFile(const std::string& path, const std::string& scheme)
{
	if (sharedInteractionController == nil)
		sharedInteractionController = [[UIDocumentInteractionController alloc] init];
	
	UIViewController* handle = (__bridge UIViewController*)(application().renderingContextHandle());
	
	CGRect presentRect = handle.view.bounds;
	presentRect.origin.x = 0.5f * presentRect.size.width - 1.0f;
	presentRect.origin.y = presentRect.size.height - 1.0f;
	presentRect.size.height = 1.0f;
	presentRect.size.width = 2.0f;
	
	sharedInteractionController.UTI = [NSString stringWithUTF8String:scheme.c_str()];
	sharedInteractionController.URL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
	[sharedInteractionController presentOptionsMenuFromRect:presentRect inView:handle.view animated:YES];
}

std::string et::selectFile(const StringList&, SelectFileMode, const std::string&)
{
	return std::string();
}