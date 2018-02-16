/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/app/application.h>
#include <et/app/applicationthreads.h>

namespace et {

/*
 * Render thread implementation
 */

RenderThread::RenderThread()
	: Thread("et-render-thread") {
}

void RenderThread::main() {
	while (running())
	{
		threading::sleepMSec(1);
	}
	log::info("Out");
}

/*
 * Background thread implementation
 */

void BackgroundThread::BackgroundRunLoop::addTask(Task* t, float delay) {
	updateTime(queryContiniousTimeInMilliSeconds());
	RunLoop::addTask(t, delay);

	if (owner->suspended())
		owner->resume();
}

BackgroundThread::BackgroundThread()
	: Thread("et-background-thread") {
	_runLoop.owner = this;
}

void BackgroundThread::main() {
	RunLoopScope runLoopScope(_runLoop);

	while (running())
	{
		if (_runLoop.hasTasks() || _runLoop.mainTimerPool()->hasObjects())
		{
			_runLoop.update(queryContiniousTimeInMilliSeconds());
		}
		else
		{
			suspend();
		}
	}
}

}
