/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	typedef unsigned long ThreadResult;
	
#if (ET_PLATFORM_ANDROID)
	typedef long ThreadId;
#else
	typedef size_t ThreadId;
#endif
	
	class ThreadPrivate;
	class Thread
	{
	public:
		static void sleep(float seconds);
		static void sleepMSec(uint64_t msec);
		
	public:
		Thread();
		Thread(bool start);
		
		virtual ~Thread();

		void run();
		void suspend();
		void resume();
		void stop();

		void waitForTermination();
		void terminate(int result = 0);

		bool running() const;
		bool suspended() const;

		ThreadId id() const;
		virtual ThreadResult main();

	private:
		Thread(const Thread&)
			{ }

		Thread& operator = (const Thread&)
			{ return *this; }

	private:
		friend class ThreadPrivate;
		ThreadPrivate* _private;
	};
}