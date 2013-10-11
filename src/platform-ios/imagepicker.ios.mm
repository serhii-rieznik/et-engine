/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/app/application.h>
#include <et/platform-ios/imagepicker.h>

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

ImagePicker::ImagePicker() :
	_private(new ImagePickerPrivate)
{
	_private->owner = this;
}

ImagePicker::~ImagePicker()
{
	delete _private;
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
	[proxy release];
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
	[_picker release];
	[super dealloc];
}

- (void)pick:(et::ImagePickerSource)source
{
	UIViewController* vc = reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	
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
		assert(cameraAvailable);
		_picker.sourceType = UIImagePickerControllerSourceTypeCamera;
	}
	else if (source == ImagePickerSource_PreferCamera)
	{
		_picker.sourceType = cameraAvailable ? UIImagePickerControllerSourceTypeCamera :
			UIImagePickerControllerSourceTypePhotoLibrary;
	}
	else
	{
		assert(0 && "Unsupported ImagePickerSource value");
	}
	
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		[_popover release];
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
		assert(0 && "Not implemented yet");
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
		
		TextureDescription::Pointer result(new TextureDescription);
		result->size.x = CGImageGetWidth(cgImage);
		result->size.y = CGImageGetHeight(cgImage);
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
		[_popover autorelease], _popover = nil;
	}
	else
	{
		assert(0 && "Not implemented yet");
	}
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		[_popover dismissPopoverAnimated:YES];
		[_popover autorelease], _popover = nil;
	}
	else
	{
		assert(0 && "Not implemented yet");
	}
}

@end

@implementation UIImagePickerWorkaround

#if defined(__IPHONE_6_0)

- (BOOL)shouldAutorotate
{
	UIViewController* vc = reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	return [vc shouldAutorotate];
}

- (NSUInteger)supportedInterfaceOrientations
{
	UIViewController* vc = reinterpret_cast<UIViewController*>(application().renderingContextHandle());
	return [vc supportedInterfaceOrientations];
}

#endif

@end
