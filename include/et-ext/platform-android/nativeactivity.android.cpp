/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

#if (ET_PLATFORM_ANDROID)

#include <et/platform-android/nativeactivity.h>

using namespace et;

JNIEnv* et::attachToThread()
{
	log::info("Attaching to thread...");
	
    JNIEnv* env = nullptr;
	et::sharedAndroidApplication()->activity->vm->AttachCurrentThread(&env, nullptr);
	
	log::info("Seems to be attached, env = %08X", env);
	
	return env;
}

JavaVM* et::getJavaVM()
{
	return et::sharedAndroidApplication()->activity->vm;
}

void et::detachFromThread()
{
	et::sharedAndroidApplication()->activity->vm->DetachCurrentThread();
}

#endif // ET_PLATFORM_ANDROID
