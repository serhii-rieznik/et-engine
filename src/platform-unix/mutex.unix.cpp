/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <errno.h>
#include <pthread.h>
#include <et/core/et.h>
#include <et/threading/mutex.h>

namespace et
{
	class MutexPrivate
	{
	public:
		pthread_mutex_t mutex;
	};
}

using namespace et;

Mutex::Mutex() :
	_private(new MutexPrivate)
{
	pthread_mutexattr_t attrib = { };
	pthread_mutexattr_init(&attrib);
	pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK);
	
	pthread_mutex_init(&_private->mutex, &attrib);
	
	pthread_mutexattr_destroy(&attrib);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&_private->mutex);
	delete _private;
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
