/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/threading/thread.h>
#include <et/app/runloop.h>

namespace et
{
	class BackgroundThread;
	
	class BackgroundRunLoop : public RunLoop
	{
	private:
		BackgroundRunLoop();
		
		void setOwner(BackgroundThread* owner);
		void addTask(Task* t, float);
		
	private:
		friend class BackgroundThread;
		BackgroundThread* _owner;
	};
	
	class BackgroundThread : public Thread
	{
	public:
		BackgroundThread();
		
		RunLoop& runLoop()
			{ return _runLoop; }
		
	private:
		ThreadResult main();
		
	private:
		BackgroundRunLoop _runLoop;
	};
}
