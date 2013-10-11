/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <et/platform-android/nativeactivity.h>
#include <et/sound/sound.h>

using namespace et;
using namespace audio;

namespace et
{
	extern android_app* sharedAndroidApplication();
	extern zip* sharedAndroidZipArchive();
	extern ALCdevice* getSharedDevice();
	extern ALCcontext* getSharedContext();
}

void Manager::nativePreInit()
{
	attachToThread();
}

void Manager::nativeInit()
{
}

void Manager::nativeRelease()
{
}

void Manager::nativePostRelease()
{
}
