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
	class ThreadPrivate;
	class Thread
	{
	public:
		using Identifier = std::thread::native_handle_type;
		
	public:
		Thread();
		Thread(bool start);
		
		virtual ~Thread();

		void run();
		void suspend();
		void resume();
		void stop();
		void join();

		void stopAndWaitForTermination();
		void terminate(int result = 0);

		bool running() const;
		bool suspended() const;

		Identifier identifier() const;
		virtual uint64_t main();

	private:
		ET_DENY_COPY(Thread)

	private:
		friend class ThreadPrivate;
		
		ET_DECLARE_PIMPL(Thread, 256)
	};
}
