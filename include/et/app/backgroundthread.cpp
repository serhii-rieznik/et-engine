/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/app/application.h>
#include <et/app/backgroundthread.h>

namespace et
{

void BackgroundRunLoop::setOwner(BackgroundThread* owner)
	{ _owner = owner; }

void BackgroundRunLoop::addTask(Task* t, float delay)
{
	updateTime(queryContiniousTimeInMilliSeconds());
	RunLoop::addTask(t, delay);
	_owner->resume();
}

BackgroundThread::BackgroundThread()
	: Thread("et-background-thread")
{
	_runLoop.setOwner(this);
}

void BackgroundThread::main()
{
	registerRunLoop(_runLoop);	
	while (running())
	{
		if (_runLoop.hasTasks() || _runLoop.firstTimerPool()->hasObjects())
		{
			_runLoop.update(queryContiniousTimeInMilliSeconds());
			threading::sleepMSec(1);
		}
		else
		{
			suspend();
		}
	}
	unregisterRunLoop(_runLoop);
}

}
