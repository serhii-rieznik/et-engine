#pragma once

#include <jni.h>
#include <libzip/zip.h>
#include <android/log.h>
#include <android_native_app_glue.h>

namespace et
{
	android_app* sharedAndroidApplication();
	zip* sharedAndroidZipArchive();
	
	JavaVM* getJavaVM();
	JNIEnv* attachToThread();
	
	void detachFromThread();
}