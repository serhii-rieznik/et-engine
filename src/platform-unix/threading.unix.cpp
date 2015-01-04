/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/threading/threading.h>

#if (ET_PLATFORM_IOS | ET_PLATFORM_MAC | ET_PLATFORM_ANDROID)

#include <pthread.h>

using namespace et;

ThreadId Threading::_mainThread;
ThreadId Threading::_renderingThread;

Threading::Threading()
{
	_mainThread = reinterpret_cast<ThreadId>(pthread_self());
	_renderingThread = _mainThread;
}

ThreadId Threading::currentThread()
{
	return reinterpret_cast<ThreadId>(pthread_self());
}

void Threading::setMainThread(ThreadId t)
{
	_mainThread = t;
}

void Threading::setRenderingThread(ThreadId t)
{
	_renderingThread = t;
}

size_t Threading::coresCount()
{
	return 0;
}

float Threading::cpuUsage()
{
	return 0.0f;
}

#endif // !ET_PLATFORM_WIN
