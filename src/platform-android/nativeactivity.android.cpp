/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/core/log.h>
#include <et/platform-android/nativeactivity.h>

using namespace et;

JNIEnv* et::attachToThread()
{
	log::info("Attaching to thread...");
	
    JNIEnv* env = nullptr; // et::sharedAndroidApplication()->activity->env;
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