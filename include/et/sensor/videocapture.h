/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/app/events.h>

namespace et
{
	struct VideoFrameData
	{
		vec2i dimensions;
		
		char* data;
		size_t dataSize;
		size_t rowSize;
		
	public:
		VideoFrameData() : data(0), dataSize(0)
			{ }
	};
	
	enum VideoCaptureFlags
	{
		VideoCaptureFlag_LockFocus = 0x01,
		VideoCaptureFlag_LockWhiteBalance = 0x02,
		VideoCaptureFlag_LockExposure = 0x04,
		
		VideoCaptureFlag_LockAllParameters =
			VideoCaptureFlag_LockFocus | VideoCaptureFlag_LockWhiteBalance | VideoCaptureFlag_LockExposure
	};
	
	enum VideoCaptureQuality
	{
		VideoCaptureQuality_Low,
		VideoCaptureQuality_Medium,
		VideoCaptureQuality_High
	};
	
	class VideoCapturePrivate;
	class VideoCapture : public EventReceiver
	{
    public:
        static bool available();
        
	public:
		VideoCapture(VideoCaptureQuality = VideoCaptureQuality_Medium);
		~VideoCapture();
        
        void run();
        void stop();
		
		void setFlags(size_t flag);
		void removeFlags(size_t flag);
		
		ET_DECLARE_EVENT1(frameDataAvailable, VideoFrameData);
		
	private:
		friend class VideoCapturePrivate;
		ET_DECLARE_PIMPL(VideoCapture, 32)
	};
}
