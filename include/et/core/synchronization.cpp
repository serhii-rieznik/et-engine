/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/criticalsection.h>
#include <et/core/mutex.h>

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
            pthread_mutexattr_t attrib = { };
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

    class MutexPrivate
    {
    private:
#   if (ET_PLATFORM_WIN)
        HANDLE _mutex;
#   else
        pthread_mutex_t mutex;
#   endif
        
    public:
        MutexPrivate()
        {
#       if (ET_PLATFORM_WIN)
            _mutex = CreateMutex(0, false, 0);
#       else
            pthread_mutexattr_t attrib = { };
            pthread_mutexattr_init(&attrib);
            pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK);
            pthread_mutex_init(&mutex, &attrib);
            pthread_mutexattr_destroy(&attrib);
#       endif
        }
        
        ~MutexPrivate()
        {
#       if (ET_PLATFORM_WIN)
            CloseHandle(_mutex);
#       else
            pthread_mutex_destroy(&mutex);
#       endif
        }
        
        void lock()
        {
#       if (ET_PLATFORM_WIN)
            WaitForSingleObject(_mutex, INFINITE);
#       else
            int result = pthread_mutex_lock(&mutex);
            if (result == EBUSY)
                log::error("Mutex already locked.");
#       endif
        }
        
        void unlock()
        {
#       if (ET_PLATFORM_WIN)
            ReleaseMutex(_mutex);
#       else
            int result = pthread_mutex_unlock(&mutex);
            if (result == EPERM)
                log::error("Mutex already unlocked or was locked from another thread.");
#       endif
        }
    };
}

using namespace et;

CriticalSection::CriticalSection()
{
	ET_PIMPL_INIT(CriticalSection)
}

CriticalSection::~CriticalSection()
{
	ET_PIMPL_FINALIZE(CriticalSection)
}

void CriticalSection::enter()
{
    _private->enter();
}

void CriticalSection::leave()
{
    _private->leave();
}

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
