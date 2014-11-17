/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	class MutexPrivate;
	class Mutex
	{
	public:
		Mutex();
		~Mutex();

		void lock();
		void unlock();

	private:
		ET_DECLARE_PIMPL(Mutex, 64)
	};

	class MutexScope
	{
	public:
		MutexScope(Mutex& mutex) : _mutex(mutex)
			{ _mutex.lock(); }

		~MutexScope()
			{ _mutex.unlock(); }
	private:
		MutexScope(const MutexScope&) : _mutex(*(static_cast<Mutex*>(0))) { }

		MutexScope& operator = (MutexScope&) 
			{ return *this; }

	private:
		Mutex& _mutex;
	};
}
