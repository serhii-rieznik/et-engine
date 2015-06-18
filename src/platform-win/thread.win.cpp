/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <iostream>
#include <et/core/tools.h>
#include <et/threading/thread.h>

#if (ET_PLATFORM_WIN)

namespace et
{
	class ThreadPrivate
	{
	public:
		ThreadPrivate::ThreadPrivate() : 
		  threadId(0), thread(nullptr), activityEvent(nullptr) { }

		static DWORD WINAPI threadProc(LPVOID lpParameter);

	public:
		DWORD threadId;
		HANDLE thread;
		HANDLE activityEvent;
		AtomicCounter running;
		AtomicCounter suspended;
	};
}

using namespace et;


DWORD WINAPI ThreadPrivate::threadProc(LPVOID lpParameter)
{
	Thread* thread = reinterpret_cast<Thread*>(lpParameter);
	thread->_private->running.retain();
	return static_cast<DWORD>(thread->main());
}

/*
 * Thread
 */

Thread::Thread()
{
	ET_PIMPL_INIT(Thread)
	_private->activityEvent = CreateEvent(0, false, false, 0);
}

Thread::Thread(bool runImmediately)
{
	ET_PIMPL_INIT(Thread)
	_private->activityEvent = CreateEvent(0, false, false, 0);

	if (runImmediately)
		run();
}

Thread::~Thread()
{
	terminate();
	CloseHandle(_private->activityEvent);

	ET_PIMPL_FINALIZE(Thread)
}

void Thread::run()
{
	if (_private->running.atomicCounterValue() == 0)
		_private->thread = CreateThread(0, 0, ThreadPrivate::threadProc, this, 0, &_private->threadId);
}

void Thread::sleep(float sec)
{
	SleepEx(static_cast<DWORD>(1000.0f * sec), TRUE);
}

void Thread::sleepMSec(uint64_t msec)
{
	SleepEx(static_cast<DWORD>(msec), TRUE);
}

ThreadResult Thread::main()
{
	return 0;
}

void Thread::suspend()
{
	if (_private->suspended.atomicCounterValue() != 0) return;

	_private->suspended.retain();
	WaitForSingleObject(_private->activityEvent, INFINITE);
}

void Thread::resume()
{
	if (_private->suspended.atomicCounterValue() == 0) return;

	_private->suspended.release();
	SetEvent(_private->activityEvent);
}

void Thread::stop()
{
	if (running())
	{
		_private->running.release();
		resume();
	}
}

void Thread::terminate(int exitCode)
{
	if (_private->running.atomicCounterValue() == 0) return;

	DWORD threadExitCode = 0;
	GetExitCodeThread(_private->thread, &threadExitCode);

	if (threadExitCode == STILL_ACTIVE)
		TerminateThread(_private->thread, exitCode);

	CloseHandle(_private->thread);
	_private->running.release();
}

bool Thread::suspended() const
{
	return (_private->suspended.atomicCounterValue() != 0);
}

bool Thread::running() const
{
	return (_private->running.atomicCounterValue() != 0);
}

ThreadId Thread::id() const
{
	return _private->threadId;
}

void Thread::waitForTermination()
{
	if (_private->thread && running())
		WaitForSingleObject(_private->thread, INFINITE);
}

void Thread::stopAndWaitForTermination()
{
	if (_private->thread && running())
	{
		stop();
		WaitForSingleObject(_private->thread, INFINITE);
	}
}

#endif // ET_PLATFORM_WIN
