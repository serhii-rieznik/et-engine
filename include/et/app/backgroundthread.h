/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/thread.h>
#include <et/app/runloop.h>

namespace et
{
	class BackgroundThread;
	
	class BackgroundRunLoop : public RunLoop
	{
	private:
		void setOwner(BackgroundThread* owner);
		void addTask(Task* t, float);
		
	private:
		friend class BackgroundThread;
		BackgroundThread* _owner = nullptr;
	};
	
	class BackgroundThread : public Thread
	{
	public:
		BackgroundThread();
		
		RunLoop& runLoop()
			{ return _runLoop; }
		
	private:
		void main();
		
	private:
		BackgroundRunLoop _runLoop;
	};
}
