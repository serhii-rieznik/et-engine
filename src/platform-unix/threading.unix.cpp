/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <pthread.h>
#include <et/threading/threading.h>

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
