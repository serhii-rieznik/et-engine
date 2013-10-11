/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/sensor/videocapture.h>

namespace et
{
	class VideoCapturePrivate
	{
	};
}

using namespace et;

VideoCapture::VideoCapture(VideoCaptureQuality) : _private(0)
{
}

VideoCapture::~VideoCapture()
{
}

void VideoCapture::run()
{
}

void VideoCapture::stop()
{
}
