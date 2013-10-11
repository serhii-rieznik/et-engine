/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <Windows.h>
#include <Pdh.h>
#include <et/threading/threading.h>

using namespace et;

ThreadId Threading::_mainThread;
ThreadId Threading::_renderingThread;

Threading::Threading()
{
	_mainThread = GetCurrentThreadId();
	_renderingThread = _mainThread;
}

ThreadId Threading::currentThread()
{
	return GetCurrentThreadId();
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
	SYSTEM_INFO info = { };
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
}

float Threading::cpuUsage()
{
	static ULONGLONG idle = 0;
	static ULONGLONG kern = 0;
	static ULONGLONG user = 0;

	ULONGLONG idle_c = 0;
	ULONGLONG kern_c = 0;
	ULONGLONG user_c = 0;
	
	GetSystemTimes(reinterpret_cast<LPFILETIME>(&idle_c), reinterpret_cast<LPFILETIME>(&kern_c), reinterpret_cast<LPFILETIME>(&user_c));

	ULONGLONG dIdle = idle_c - idle;
	ULONGLONG dSys = (user_c - user) + (kern_c - kern);

	idle = idle_c;
	kern = kern_c;
	user = user_c;

	return static_cast<float>(dSys - dIdle) / static_cast<float>(dSys) * 100.0f;
}