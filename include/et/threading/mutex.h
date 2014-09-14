/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

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
		MutexPrivate* _private;
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
