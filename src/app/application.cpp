/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/threading/threading.h>
#include <et/app/application.h>

using namespace et;

namespace et
{
	uint32_t randomInteger(uint32_t limit);
}

IApplicationDelegate* et::Application::_delegate = nullptr;

Application::Application() : _renderContext(nullptr), _exitCode(0),
	_lastQueuedTimeMSec(queryContiniousTimeInMilliSeconds()),
	_fpsLimitMSec(0), _fpsLimitMSecFractPart(0), _postResizeOnActivate(false)
{
	threading();

	log::addOutput(log::ConsoleOutput::Pointer::create());
	
	delegate()->setApplicationParameters(_parameters);

	platformInit();
	platformActivate();
	
	_backgroundThread.run();
}

Application::~Application()
{
	_running = false;

	_backgroundThread.stop();
	_backgroundThread.waitForTermination();
	
	platformDeactivate();
	platformFinalize();
}

IApplicationDelegate* Application::delegate()
{
	if (_delegate == nullptr)
	{
		_delegate = initApplicationDelegate();
		ET_ASSERT(_delegate);
		_identifier = _delegate->applicationIdentifier();
	}
    
	return _delegate;
}

int Application::run(int argc, char* argv[])
{
	for (int i = 0; i < argc; ++i)
		_launchParameters.push_back(argv[i]);

	return platformRun(argc, argv);
}

void Application::performRendering()
{
	_renderContext->beginRender();
	_delegate->render(_renderContext);
	_renderContext->endRender();
}

void Application::idle()
{
	ET_ASSERT(_running);

	uint64_t currentTime = queryContiniousTimeInMilliSeconds();
	uint64_t elapsedTime = currentTime - _lastQueuedTimeMSec;

	if (elapsedTime >= _fpsLimitMSec)
	{
		if (!_suspended)
		{
			_runLoop.update(currentTime);
			_delegate->idle(_runLoop.mainTimerPool()->actualTime());
			performRendering();
		}
		
		_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
	}
	else 
	{
		uint64_t sleepInterval = (_fpsLimitMSec - elapsedTime) +
			(randomInteger(1000) > _fpsLimitMSecFractPart ? 0 : static_cast<uint64_t>(-1));
		
		Thread::sleepMSec(sleepInterval);
	}
}

void Application::setFrameRateLimit(size_t value)
{
	_fpsLimitMSec = (value == 0) ? 0 : 1000 / value;
	_fpsLimitMSecFractPart = (value == 0) ? 0 : (1000000 / value - 1000 * _fpsLimitMSec);
}

void Application::setActive(bool active)
{
	if (!_running || (_active == active)) return;

	_active = active;
	
	if (active)
	{
		if (_suspended)
			resume();

		_delegate->applicationWillActivate();
		
		if (_postResizeOnActivate)
		{
			_delegate->applicationWillResizeContext(_renderContext->sizei());
			_postResizeOnActivate = false;
		}
		
		platformActivate();
	}
	else
	{
		_delegate->applicationWillDeactivate();
		platformDeactivate();
		
		if (_parameters.shouldSuspendOnDeactivate)
			suspend();
	}
}

void Application::contextResized(const vec2i& size)
{
	if (_running)
	{
		if (_active)
			_delegate->applicationWillResizeContext(size);
		else
			_postResizeOnActivate = true;
	}
}

float Application::cpuLoad() const
{
	return Threading::cpuUsage();
}

void Application::suspend()
{
	if (_suspended) return;

	delegate()->applicationWillSuspend();
	_runLoop.pause();

	platformSuspend();
	
	_suspended = true;
}

void Application::resume()
{
	ET_ASSERT(_suspended && "Should be suspended.");

	delegate()->applicationWillResume();
	
	_suspended = false;

	platformResume();

	_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
	_runLoop.update(_lastQueuedTimeMSec);
	_runLoop.resume();
}

void Application::stop()
{
	_running = false;
}

void Application::terminated()
{
	_delegate->applicationWillTerminate();
}
