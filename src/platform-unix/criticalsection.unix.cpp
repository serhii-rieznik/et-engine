/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/threading/criticalsection.h>

#if (ET_PLATFORM_IOS | ET_PLATFORM_MAC | ET_PLATFORM_ANDROID)

#include <errno.h>
#include <pthread.h>

namespace et
{
	class CriticalSectionPrivate
	{
	public:
		pthread_mutex_t mutex;
	};
}

using namespace et;

CriticalSection::CriticalSection()
{
	ET_PIMPL_INIT(CriticalSection)
	
	pthread_mutexattr_t attrib = { };
	pthread_mutexattr_init(&attrib);
	pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE);
	
	pthread_mutex_init(&_private->mutex, &attrib);
	
	pthread_mutexattr_destroy(&attrib);
}

CriticalSection::~CriticalSection()
{
	pthread_mutex_destroy(&_private->mutex);

	ET_PIMPL_FINALIZE(CriticalSection)
}

void CriticalSection::enter()
{
	int result = pthread_mutex_lock(&_private->mutex);
	if (result == EBUSY)
		log::warning("Mutex already locked.");
}

void CriticalSection::leave()
{
	int result = pthread_mutex_unlock(&_private->mutex);
	if (result == EPERM)
		log::warning("Mutex already unlocked or was locked from another thread.");
}

#endif // !ET_PLATFORM_WIN
