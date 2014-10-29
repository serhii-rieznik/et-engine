/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/opengl/opengl.h>
#include <et/app/application.h>

#if (ET_PLATFORM_IOS)

#include <UIKit/UIImage.h>
#include <UIKit/UIImagePickerController.h>
#include <UIKit/UIPopoverController.h>
#include <et/platform-ios/imagepicker.h>
#include <et/platform-apple/apple.h>

@interface UIImagePickerWorkaround : UIImagePickerController

@end

@interface ImagePickerProxy : NSObject<UIImagePickerControllerDelegate, UINavigationControllerDelegate>
{
	UIImagePickerWorkaround* _picker;
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
		ImagePicker* owner;
		ImagePickerProxy* proxy;
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
	@try
	{
		[proxy pick:s];
	}
	@catch (NSException* e)
	{
		NSLog(@"%@, %@", e, [e callStackSymbols]);
		abort();
	}
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

- (void)pick:(et::ImagePickerSource)source
{
	void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
	UIViewController* vc = (__bridge UIViewController*)vcptr;
	
	if (_picker == nil)
	{
		_picker = [[UIImagePickerWorkaround alloc] init];
		_picker.delegate = self;
	}
	
	BOOL cameraAvailable = [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera];
	if (source == ImagePickerSource_PhotoAlbum)
	{
		_picker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
	}
	else if (source == ImagePickerSource_Camera)
	{
		ET_ASSERT(cameraAvailable);
		_picker.sourceType = UIImagePickerControllerSourceTypeCamera;
	}
	else if (source == ImagePickerSource_PreferCamera)
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
		
		CGRect frame = vc.view.bounds;
		frame.origin.x = 0.5f * frame.size.width;
		frame.origin.y = frame.size.height - 1.0f;
		frame.size = CGSizeMake(1.0f, 1.0f);
		
		[_popover presentPopoverFromRect:frame inView:vc.view
			   permittedArrowDirections:UIPopoverArrowDirectionDown animated:YES];
	}
	else
	{
		ET_FAIL("Not implemented yet");
	}
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
	UIImage* image = [info objectForKey:UIImagePickerControllerOriginalImage];
	if (image == nil)
	{
		NSURL* imageUrl = [info objectForKey:UIImagePickerControllerReferenceURL];
		if (imageUrl != nil)
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
	}
	
	if (image != nil)
	{
		CGImageRef cgImage = [image CGImage];
		
		TextureDescription::Pointer result = TextureDescription::Pointer::create();
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
		
#if (!ET_OBJC_ARC_ENABLED)
		[_popover autorelease];
#endif
		_popover = nil;
	}
	else
	{
		ET_FAIL("Not implemented yet");
	}
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		[_popover dismissPopoverAnimated:YES];
		
#if (!ET_OBJC_ARC_ENABLED)
		[_popover autorelease];
#endif
		_popover = nil;
	}
	else
	{
		ET_FAIL("Not implemented yet");
	}
}

@end

@implementation UIImagePickerWorkaround

- (BOOL)shouldAutorotate
{
	void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
	UIViewController* vc = (__bridge UIViewController*)vcptr;
	
	return [vc shouldAutorotate];
}

- (NSUInteger)supportedInterfaceOrientations
{
	void* vcptr = reinterpret_cast<void*>(application().renderingContextHandle());
	UIViewController* vc = (__bridge UIViewController*)vcptr;
	
	return [vc supportedInterfaceOrientations];
}

@end

#endif // ET_PLATFORM_IOS
