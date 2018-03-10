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

void RenderThread::init(const RenderInterface::Pointer& renderer) {
	_renderer = renderer;
}

void RenderThread::main() {
	while (running())
	{
		if (_renderer.valid() && _renderer->parameters().multithreadingEnabled)
		{
			_renderer->present();
		}
		else
		{
			threading::sleepMSec(0);
		}
	}
	log::info("Out");
}

/*
 * Background thread implementation
 */

void BackgroundThread::BackgroundRunLoop::addTask(Task* t, float delay) {
	updateTime(queryContinuousTimeInMilliSeconds());
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
			_runLoop.update(queryContinuousTimeInMilliSeconds());
		}
		else
		{
			suspend();
		}
	}
}

}
