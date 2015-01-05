/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/app/application.h>

#if (ET_PLATFORM_IOS)

#include <UIKit/UIImage.h>
#include <UIKit/UIImagePickerController.h>
#include <UIKit/UIPopoverController.h>
#include <ImageIO/CGImageProperties.h>

#include <et/app/applicationnotifier.h>
#include <et/opengl/opengl.h>
#include <et/platform-ios/imagepicker.h>
#include <et/platform-apple/apple.h>

@interface UIImagePickerController(Fixes)

@end

@interface ImagePickerProxy : NSObject<UIImagePickerControllerDelegate, UINavigationControllerDelegate, UIPopoverControllerDelegate>
{
	UIImagePickerController* _picker;
	UIPopoverController* _popover;
	et::ImagePickerPrivate* _p;
}

- (id)initWithPrivate:(et::ImagePickerPrivate*)p;
- (void)pick:(et::ImagePickerSource)source;

@end

namespace et
{
	class ImagePickerPrivate
	{
	public:
		ImagePickerPrivate();
		~ImagePickerPrivate();
		
		void pick(ImagePickerSource s);
		
	public:
		ImagePicker* owner = nullptr;
		ImagePickerProxy* proxy = nullptr;
	};
}

using namespace et;

ImagePicker::ImagePicker()
{
	ET_PIMPL_INIT(ImagePicker)
	_private->owner = this;
}

ImagePicker::~ImagePicker()
{
	ET_PIMPL_FINALIZE(ImagePicker)
}

void ImagePicker::selectImage(ImagePickerSource source)
{
	_private->pick(source);
}

/*
 * Image Picker Private
 */
ImagePickerPrivate::ImagePickerPrivate()
{
	proxy = [[ImagePickerProxy alloc] initWithPrivate:this];
}

ImagePickerPrivate::~ImagePickerPrivate()
{
	ET_OBJC_RELEASE(proxy)
}

void ImagePickerPrivate::pick(ImagePickerSource s)
{
	dispatch_async(dispatch_get_main_queue(), ^
	{
		[proxy pick:s];
	});
}

/*
 * Image Picker Proxy
 */

@implementation ImagePickerProxy

- (id)initWithPrivate:(et::ImagePickerPrivate*)p
{
	self = [super init];
	if (self)
	{
		_p = p;
		_picker = nil;
		_popover = nil;
	}
	return self;
}

- (void)dealloc
{
	ET_OBJC_RELEASE(_picker)
	
#if (!ET_OBJC_ARC_ENABLED)
	[super dealloc];
#endif
}

- (void)updatePickerCameraOrientation
{
	if (_picker.sourceType == UIImagePickerControllerSourceTypeCamera)
	{
		void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
		UIViewController* vc = (__bridge UIViewController*)vcptr;

		switch ([vc interfaceOrientation])
		{
			case UIInterfaceOrientationPortraitUpsideDown:
				_picker.cameraViewTransform = CGAffineTransformMakeRotation(PI);
				break;

			case UIInterfaceOrientationLandscapeLeft:
				_picker.cameraViewTransform = CGAffineTransformMakeRotation(HALF_PI);
				break;

			case UIInterfaceOrientationLandscapeRight:
				_picker.cameraViewTransform = CGAffineTransformMakeRotation(-HALF_PI);
				break;
				
			default:
				_picker.cameraViewTransform = CGAffineTransformIdentity;
		}
	}
}

- (void)popoverController:(UIPopoverController *)popoverController willRepositionPopoverToRect:(inout CGRect *)rect inView:(inout UIView **)view
{
	UIView* aView = *view;
	*rect = CGRectMake(0.5f * aView.bounds.size.width, 0.5f * aView.bounds.size.height, 1.0f, 1.0f);
	[self updatePickerCameraOrientation];
}

- (void)pick:(et::ImagePickerSource)source
{
	void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
	UIViewController* vc = (__bridge UIViewController*)vcptr;
	
	if (_picker == nil)
	{
		_picker = [[UIImagePickerController alloc] init];
		_picker.delegate = self;
	}
	
	BOOL cameraAvailable = [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera];
	
	if (source == ImagePickerSource::PhotoAlbum)
	{
		_picker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
	}
	else if (source == ImagePickerSource::Camera)
	{
		ET_ASSERT(cameraAvailable);
		_picker.sourceType = UIImagePickerControllerSourceTypeCamera;
	}
	else if (source == ImagePickerSource::PreferCamera)
	{
		_picker.sourceType = cameraAvailable ? UIImagePickerControllerSourceTypeCamera :
			UIImagePickerControllerSourceTypePhotoLibrary;
	}
	else
	{
		ET_FAIL_FMT("Unsupported ImagePickerSource value: %d", source);
	}
	
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		ET_OBJC_RELEASE(_popover)

		_popover = [[UIPopoverController alloc] initWithContentViewController:_picker];
		_popover.delegate = self;
		
		CGRect presentFrame = CGRectMake(0.5f * vc.view.bounds.size.width, 0.5f * vc.view.bounds.size.height, 1.0f, 1.0f);
		
		[_popover presentPopoverFromRect:presentFrame inView:vc.view
			permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
		
		[self updatePickerCameraOrientation];
	}
	else
	{
		[vc presentViewController:_picker animated:YES completion:nil];
	}
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
	UIImage* image = [info objectForKey:UIImagePickerControllerOriginalImage];
	NSURL* imageUrl = [info objectForKey:UIImagePickerControllerReferenceURL];
	
	if ((image == nil) && (imageUrl != nil))
	{
		NSError* error = nil;
		NSData* imageData = [NSData dataWithContentsOfURL:imageUrl options:NSDataReadingMappedIfSafe error:&error];
		if (imageData != nil)
		{
			image = [UIImage imageWithData:imageData];
		}
		else if (error != nil)
		{
			NSLog(@"Error reading image from URL %@ - %@", imageUrl, error);
		}
	}
	
	if (image != nil)
	{
		std::string origin;
		
		if (imageUrl == nil)
		{
			NSTimeInterval timestamp = [[NSDate date] timeIntervalSince1970] / 3600.0;
			origin = [[NSString stringWithFormat:@"image%g", timestamp] cStringUsingEncoding:NSUTF8StringEncoding];
		}
		else
		{
			NSString* assetPath = [imageUrl absoluteString];
			
			NSRange assetIdPos = [assetPath rangeOfString:@"?id="];
			NSRange extPos = [assetPath rangeOfString:@"&ext"];
			
			auto assetIdLength = extPos.location - assetIdPos.location - assetIdPos.length;
			
			NSString* assetId = [assetPath substringWithRange:NSMakeRange(assetIdPos.location + assetIdPos.length, assetIdLength)];
			assetPath = [NSString stringWithFormat:@"image%@", assetId];
			
			origin = [assetPath cStringUsingEncoding:NSUTF8StringEncoding];
		}
		
		CGImageRef cgImage = [image CGImage];
		TextureDescription::Pointer result = TextureDescription::Pointer::create();
		result->setOrigin(origin);
		result->size.x = (int)CGImageGetWidth(cgImage);
		result->size.y = (int)CGImageGetHeight(cgImage);
		result->target = GL_TEXTURE_2D;
		result->internalformat = GL_RGBA;
		result->format = GL_RGBA;
		result->type = GL_UNSIGNED_BYTE;
		result->compressed = false;
		result->bitsPerPixel = CGImageGetBitsPerPixel(cgImage);
		result->channels = result->bitsPerPixel / CGImageGetBitsPerComponent(cgImage);
		result->mipMapCount = 1;
		result->layersCount = 1;
		
		size_t bytesPerRow = CGImageGetBytesPerRow(cgImage);
		
		CFDataRef data = CGDataProviderCopyData(CGImageGetDataProvider(cgImage));
		result->data = BinaryDataStorage(static_cast<size_t>(CFDataGetLength(data)));
		
		const UInt8* sourcePtr = CFDataGetBytePtr(data) + result->data.dataSize() - bytesPerRow;
		char* destPtr = result->data.binary();
		for (size_t i = 0; i < result->size.y; ++i)
		{
			etCopyMemory(destPtr, sourcePtr, bytesPerRow);
			destPtr += bytesPerRow;
			sourcePtr -= bytesPerRow;
		}
		CFRelease(data);
		
		_p->owner->imageSelected.invoke(result);
	}
	else
	{
		_p->owner->cancelled.invoke();
	}
	
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		[_popover dismissPopoverAnimated:YES];
		(void)ET_OBJC_AUTORELEASE(_popover);
		_popover = nil;
	}
	else
	{
		void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
		UIViewController* vc = (__bridge UIViewController*)vcptr;
		[vc dismissViewControllerAnimated:YES completion:nil];
	}
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		[_popover dismissPopoverAnimated:YES];
		(void)ET_OBJC_AUTORELEASE(_popover);
		_popover = nil;
	}
	else
	{
		void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
		UIViewController* vc = (__bridge UIViewController*)vcptr;
		[vc dismissViewControllerAnimated:YES completion:nil];
	}
}

@end

@implementation UIImagePickerController(Fixes)

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (UIViewController *)childViewControllerForStatusBarHidden
{
	return nil;
}

- (BOOL)shouldAutorotate
{
	if (self.sourceType == UIImagePickerControllerSourceTypeCamera)
		return NO;
	
	void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
	UIViewController* vc = (__bridge UIViewController*)vcptr;
	
	return [vc shouldAutorotate];
}

- (NSUInteger)supportedInterfaceOrientations
{
	if (self.sourceType == UIImagePickerControllerSourceTypeCamera)
		return UIInterfaceOrientationMaskPortrait;
	
	void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
	UIViewController* vc = (__bridge UIViewController*)vcptr;
	
	return [vc supportedInterfaceOrientations];
}

@end

#endif // ET_PLATFORM_IOS
