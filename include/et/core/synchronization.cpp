/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/criticalsection.h>

#if (ET_PLATFORM_WIN)
#   include <Windows.h>
#else
#   include <errno.h>
#   include <pthread.h>
#endif

namespace et
{
class CriticalSectionPrivate
{
private:
#   if (ET_PLATFORM_WIN)
	RTL_CRITICAL_SECTION _cs;
#   else
	pthread_mutex_t _cs;
#   endif
public:

	CriticalSectionPrivate()
	{
#       if (ET_PLATFORM_WIN)
		InitializeCriticalSection(&_cs);
#       else
		pthread_mutexattr_t attrib = {};
		pthread_mutexattr_init(&attrib);
		pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&_cs, &attrib);
		pthread_mutexattr_destroy(&attrib);
#       endif
	}

	~CriticalSectionPrivate()
	{
#       if (ET_PLATFORM_WIN)
		DeleteCriticalSection(&_cs);
#       else
		pthread_mutex_destroy(&_cs);
#       endif
	}

	void enter()
	{
#       if (ET_PLATFORM_WIN)
		EnterCriticalSection(&_cs);
#       else
		int result = pthread_mutex_lock(&_cs);
		if (result == EBUSY)
			log::warning("Mutex already locked.");
#       endif
	}

	void leave()
	{
#       if (ET_PLATFORM_WIN)
		LeaveCriticalSection(&_cs);
#       else
		int result = pthread_mutex_unlock(&_cs);
		if (result == EPERM)
			log::warning("Mutex already unlocked or was locked from another thread.");
#       endif
	}
};
}

using namespace et;

CriticalSection::CriticalSection()
{
	ET_PIMPL_INIT(CriticalSection);
}

CriticalSection::~CriticalSection()
{
	ET_PIMPL_FINALIZE(CriticalSection);
}

void CriticalSection::enter()
{
	_private->enter();
}

void CriticalSection::leave()
{
	_private->leave();
}
