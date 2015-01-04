/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/threading/mutex.h>

#if (ET_PLATFORM_WIN)

#include <Windows.h>

namespace et
{
	class MutexPrivate
	{
	public:
		MutexPrivate()
			{ _mutex = CreateMutex(0, false, 0); }

		~MutexPrivate()
			{ CloseHandle(_mutex); }

		void lock()
			{ WaitForSingleObject(_mutex, INFINITE); }

		void unlock()
			{ ReleaseMutex(_mutex); }

	private:
		HANDLE _mutex;
	};
}

using namespace et;

Mutex::Mutex()
{
	ET_PIMPL_INIT(Mutex)
}

Mutex::~Mutex()
{
	ET_PIMPL_FINALIZE(Mutex)
}

void Mutex::lock()
{
	_private->lock();
}

void Mutex::unlock()
{
	_private->unlock();
}

#endif // ET_PLATFORM_WIN
