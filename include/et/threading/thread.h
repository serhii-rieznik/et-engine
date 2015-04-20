/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
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

		void stopAndWaitForTermination();
		void terminate(int result = 0);

		bool running() const;
		bool suspended() const;

		ThreadId id() const;
		virtual ThreadResult main();

	private:
		ET_DENY_COPY(Thread)

		void waitForTermination(); // deprecated

	private:
		friend class ThreadPrivate;
		
		ET_DECLARE_PIMPL(Thread, 256)
	};
}
