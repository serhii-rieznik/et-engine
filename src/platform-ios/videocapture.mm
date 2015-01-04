/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/sensor/videocapture.h>

#if (ET_PLATFORM_IOS)

#import <CoreVideo/CoreVideo.h>
#import <AVFoundation/AVFoundation.h>
#include <et/platform-apple/apple.h>

@class VideoCaptureProxy;

namespace et
{
	class VideoCapturePrivate
	{
	public:
		VideoCapturePrivate(VideoCapture* owner, VideoCaptureQuality q);
		~VideoCapturePrivate();
		
		void handleSampleBuffer(CMSampleBufferRef);
        void run();
        void stop();

		void setFocusLocked(bool isLocked);
		void setWhiteBalanceLocked(bool isLocked);
		void setExposureLocked(bool isLocked);
		
		void beginConfiguration();
		void endConfiguration();
		
	private:
		VideoCapture* _owner;
		VideoCaptureProxy* _proxy;
		AVCaptureSession* _session;
		size_t _configurationCounter;
	};
}

using namespace et;

@interface VideoCaptureProxy : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
	VideoCapturePrivate* _p;
}

- (id)initWithVideoCapturePrivate:(VideoCapturePrivate*)p;

- (void)captureOutput:(AVCaptureOutput *)captureOutput
	didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection;

@end

@implementation VideoCaptureProxy

- (id)initWithVideoCapturePrivate:(VideoCapturePrivate*)p
{
	self = [super init];
	if (self)
	{
		_p = p;
	}
	return self;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
	   fromConnection:(AVCaptureConnection *)connection
{
	_p->handleSampleBuffer(sampleBuffer);
}

@end

VideoCapture::VideoCapture(VideoCaptureQuality q)
{
	ET_PIMPL_INIT(VideoCapture, this, q);
}

VideoCapture::~VideoCapture()
{
	ET_PIMPL_FINALIZE(VideoCapture)
}

void VideoCapture::run()
    { _private->run(); }

void VideoCapture::stop()
    { _private->stop(); }

void VideoCapture::setFlags(size_t flags)
{
	if (flags)
		_private->beginConfiguration();
	
	if ((flags & VideoCaptureFlag_LockFocus) == VideoCaptureFlag_LockFocus)
		_private->setFocusLocked(true);
	if ((flags & VideoCaptureFlag_LockExposure) == VideoCaptureFlag_LockExposure)
		_private->setExposureLocked(true);
	if ((flags & VideoCaptureFlag_LockWhiteBalance) == VideoCaptureFlag_LockWhiteBalance)
		_private->setWhiteBalanceLocked(true);
	
	if (flags)
		_private->endConfiguration();
}

void VideoCapture::removeFlags(size_t flags)
{
	if (flags)
		_private->beginConfiguration();
	
	if ((flags & VideoCaptureFlag_LockFocus) == VideoCaptureFlag_LockFocus)
		_private->setFocusLocked(false);
	if ((flags & VideoCaptureFlag_LockExposure) == VideoCaptureFlag_LockExposure)
		_private->setExposureLocked(false);
	if ((flags & VideoCaptureFlag_LockWhiteBalance) == VideoCaptureFlag_LockWhiteBalance)
		_private->setWhiteBalanceLocked(false);
	
	if (flags)
		_private->endConfiguration();
}

bool VideoCapture::available()
{
    return [[AVCaptureDevice devices] count] > 0;
}

VideoCapturePrivate::VideoCapturePrivate(VideoCapture* owner, VideoCaptureQuality q) :
	_owner(owner)
{
	NSArray* devices = [AVCaptureDevice devices];
    if ([devices count] == 0) return;
    
	_proxy = [[VideoCaptureProxy alloc] initWithVideoCapturePrivate:this];

	_session = [[AVCaptureSession alloc] init];
	
	if (q == VideoCaptureQuality_Low)
		_session.sessionPreset = AVCaptureSessionPresetLow;
	else if (q == VideoCaptureQuality_High)
		_session.sessionPreset = AVCaptureSessionPresetHigh;
	else
		_session.sessionPreset = AVCaptureSessionPresetMedium;

	NSError* error = nil;
	AVCaptureDeviceInput* _input = [AVCaptureDeviceInput deviceInputWithDevice:[devices objectAtIndex:0] error:&error];
	if (_input && [_session canAddInput:_input])
		[_session addInput:_input];
	
	AVCaptureVideoDataOutput* _output = [[AVCaptureVideoDataOutput alloc] init];
	_output.alwaysDiscardsLateVideoFrames = YES;
	_output.videoSettings = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:kCVPixelFormatType_32BGRA], kCVPixelBufferPixelFormatTypeKey, nil];
	
	[_output setSampleBufferDelegate:_proxy queue:dispatch_get_main_queue()];
	[_session addOutput:_output];
	[_session startRunning];
}

VideoCapturePrivate::~VideoCapturePrivate()
{
    stop();
	
	ET_OBJC_RELEASE(_session)
	ET_OBJC_RELEASE(_proxy)
}

void VideoCapturePrivate::handleSampleBuffer(CMSampleBufferRef sampleBuffer)
{
	CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	ET_ASSERT(CVPixelBufferLockBaseAddress(imageBuffer, 0) == kCVReturnSuccess);
	
	VideoFrameData data;
	
	data.dimensions = vec2i(static_cast<int>(CVPixelBufferGetWidth(imageBuffer)),
		static_cast<int>(CVPixelBufferGetHeight(imageBuffer)));
	
	data.data = static_cast<char*>(CVPixelBufferGetBaseAddress(imageBuffer));
	data.dataSize = CVPixelBufferGetDataSize(imageBuffer);
	data.rowSize = CVPixelBufferGetBytesPerRow(imageBuffer);
	_owner->frameDataAvailable.invoke(data);
	
	CVPixelBufferUnlockBaseAddress(imageBuffer, 0);
}

void VideoCapturePrivate::run()
{
	if (!_session.running)
		[_session startRunning];
}

void VideoCapturePrivate::stop()
{
	if (_session.running)
		[_session stopRunning];
}

void VideoCapturePrivate::setFocusLocked(bool isLocked)
{
	if (_session == nullptr) return;

	beginConfiguration();
	
	NSArray* devices = [AVCaptureDevice devices];
	NSError* error = nil;
	
	for (AVCaptureDevice* device in devices)
	{
		if (([device hasMediaType:AVMediaTypeVideo]) && ([device position] == AVCaptureDevicePositionBack))
		{
			[device lockForConfiguration:&error];
			if (isLocked)
			{
				if ([device isFocusModeSupported:AVCaptureFocusModeLocked])
					device.focusMode = AVCaptureFocusModeLocked;
			}
			else
			{
				if ([device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus])
					device.focusMode = AVCaptureFocusModeContinuousAutoFocus;
			}
			[device unlockForConfiguration];
		}
	}
	
	endConfiguration();
}

void VideoCapturePrivate::setWhiteBalanceLocked(bool isLocked)
{
	if (_session == nullptr) return;
	
	beginConfiguration();
	
	NSArray* devices = [AVCaptureDevice devices];
	NSError* error = nil;
	
	for (AVCaptureDevice* device in devices)
	{
		if (([device hasMediaType:AVMediaTypeVideo]) && ([device position] == AVCaptureDevicePositionBack))
		{
			[device lockForConfiguration:&error];
			if (isLocked)
			{
				if ([device isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeLocked])
					device.whiteBalanceMode = AVCaptureWhiteBalanceModeLocked;
			}
			else
			{
				if ([device isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance])
					device.whiteBalanceMode = AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance;
			}
			[device unlockForConfiguration];
		}
	}
	
	endConfiguration();
}

void VideoCapturePrivate::setExposureLocked(bool isLocked)
{
	if (_session == nullptr) return;

	beginConfiguration();
	
	NSArray* devices = [AVCaptureDevice devices];
	NSError* error = nil;
	
	for (AVCaptureDevice* device in devices)
	{
		if (([device hasMediaType:AVMediaTypeVideo]) && ([device position] == AVCaptureDevicePositionBack))
		{
			[device lockForConfiguration:&error];
			if (isLocked)
			{
				if ([device isExposureModeSupported:AVCaptureExposureModeLocked])
					device.exposureMode = AVCaptureExposureModeLocked;
			}
			else
			{
				if ([device isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure])
					device.exposureMode = AVCaptureExposureModeContinuousAutoExposure;
			}
			[device unlockForConfiguration];
		}
	}
	
	endConfiguration();
}

void VideoCapturePrivate::beginConfiguration()
{
	if (_configurationCounter == 0)
		[_session beginConfiguration];
	
	++_configurationCounter;
}

void VideoCapturePrivate::endConfiguration()
{
	--_configurationCounter;
	
	if (_configurationCounter == 0)
		[_session commitConfiguration];
}

#endif // ET_PLATFORM_IOS
