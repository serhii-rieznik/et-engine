/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/threading/thread.h>

namespace et
{
	class Threading : public Singleton<Threading>
	{
	public:
		static size_t coresCount();
		static float cpuUsage();
		
		static ThreadId currentThread();
		
		static ThreadId mainThread()
			{ return _mainThread; }
		
		static ThreadId renderingThread()
			{ return _renderingThread; }
		
		static void setMainThread(ThreadId);
		static void setRenderingThread(ThreadId);

	private:
		Threading();
		ET_SINGLETON_COPY_DENY(Threading)

	private:
		static ThreadId _mainThread;
		static ThreadId _renderingThread;
	};

	inline Threading& threading()
		{ return Threading::instance(); }
}
