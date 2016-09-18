/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/sensor/videocapture.h>

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
