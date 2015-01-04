/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/threading/mutex.h>

#if (ET_PLATFORM_IOS | ET_PLATFORM_MAC | ET_PLATFORM_ANDROID)

#include <errno.h>
#include <pthread.h>

namespace et
{
	class MutexPrivate
	{
	public:
		pthread_mutex_t mutex;
	};
}

using namespace et;

Mutex::Mutex()
{
	ET_PIMPL_INIT(Mutex)
	
	pthread_mutexattr_t attrib = { };
	pthread_mutexattr_init(&attrib);
	pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK);
	
	pthread_mutex_init(&_private->mutex, &attrib);
	
	pthread_mutexattr_destroy(&attrib);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&_private->mutex);
	
	ET_PIMPL_FINALIZE(Mutex)
}

void Mutex::lock()
{
	int result = pthread_mutex_lock(&_private->mutex);
	if (result == EBUSY)
		log::error("Mutex already locked.");
}

void Mutex::unlock()
{
	int result = pthread_mutex_unlock(&_private->mutex);
	if (result == EPERM)
		log::error("Mutex already unlocked or was locked from another thread.");
}

#endif // !ET_PLATFORM_WIN
