/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/threading/threading.h>
#include <et/rendering/rendercontext.h>
#include <et/app/application.h>

using namespace et;

namespace et
{
	uint32_t randomInteger(uint32_t limit);
}

Application::Application()
{
	sharedObjectFactory();
	log::addOutput(log::ConsoleOutput::Pointer::create());

	_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
	
	threading();

	delegate()->setApplicationParameters(_parameters);

	platformInit();
	platformActivate();
	
	_backgroundThread.run();
}

Application::~Application()
{
	_running = false;
	
	platformDeactivate();
	platformFinalize();
}

IApplicationDelegate* Application::delegate()
{
	if (_delegate == nullptr)
	{
		_delegate = initApplicationDelegate();
		ET_ASSERT(_delegate);

#	if !defined(ET_EMBEDDED_APPLICATION)
		_identifier = _delegate->applicationIdentifier();
#	endif
	}
    
	return _delegate;
}

int Application::run(int argc, char* argv[])
{
#if (ET_DEBUG)
#	if defined(ET_CONSOLE_APPLICATION)
		log::info("[et-engine] Version: %d.%d, running console application in debug mode.",
			ET_MAJOR_VERSION, ET_MINOR_VERSION);
#	else
		log::info("[et-engine] Version: %d.%d, running in debug mode.",
			ET_MAJOR_VERSION, ET_MINOR_VERSION);
#	endif
#endif
	
	for (int i = 0; i < argc; ++i)
		_launchParameters.push_back(argv[i]);

	return platformRun(argc, argv);
}

void Application::enterRunLoop()
{
	_running = true;
	
	_standardPathResolver.setRenderContext(_renderContext);
	delegate()->applicationDidLoad(_renderContext);
	
#if defined(ET_CONSOLE_APPLICATION)
	
	setActive(true);
	
	while (_running)
		performUpdateAndRender();
	
	terminated();
#else
	
	_renderContext->init();
	setActive(true);
	
#endif
}

void Application::performRendering()
{
#if defined(ET_CONSOLE_APPLICATION)
	
#else
	_renderContext->beginRender();
	_delegate->render(_renderContext);
	_renderContext->endRender();
#endif
}

bool Application::shouldPerformRendering()
{
	uint64_t currentTime = queryContiniousTimeInMilliSeconds();
	
#if defined(ET_EMBEDDED_APPLICATION)
	
	_lastQueuedTimeMSec = currentTime;
	return true;
	
#else
	
	uint64_t elapsedTime = currentTime - _lastQueuedTimeMSec;
	if (elapsedTime < _fpsLimitMSec)
	{
		uint64_t sleepInterval = (_fpsLimitMSec - elapsedTime) +
			(randomInteger(1000) > _fpsLimitMSecFractPart ? 0 : static_cast<uint64_t>(-1));
		
		Thread::sleepMSec(sleepInterval);
		
		return false;
	}
	_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
	
	return !_suspended;
	
#endif
}

void Application::performUpdateAndRender()
{
	ET_ASSERT(_running && !_suspended);
	
	_runLoop.update(_lastQueuedTimeMSec);
	_delegate->idle(_runLoop.firstTimerPool()->actualTime());
	
#if !defined(ET_CONSOLE_APPLICATION)
	performRendering();
#endif
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

std::string Application::resolveFileName(const std::string& path)
{
	std::string result;
	
	if (_customPathResolver.valid())
		result = _customPathResolver->resolveFilePath(path);
	
	if (!fileExists(result))
		result = _standardPathResolver.resolveFilePath(path);
	
	return result;
}

std::string Application::resolveFolderName(const std::string& path)
{
	std::string result;
	
	if (_customPathResolver.valid())
		result = _customPathResolver->resolveFolderPath(path);
	
	if (!folderExists(result))
		result = _standardPathResolver.resolveFolderPath(path);
	
	return result;
}

std::set<std::string> Application::resolveFolderNames(const std::string& path)
{
	std::set<std::string> result;
	
	if (_customPathResolver.valid())
		result = _customPathResolver->resolveFolderPaths(path);
	
	auto standard = _standardPathResolver.resolveFolderPaths(path);
	result.insert(standard.begin(), standard.end());
	
	return result;
}

void Application::pushSearchPath(const std::string& path)
{
	_standardPathResolver.pushSearchPath(path);
}

void Application::pushRelativeSearchPath(const std::string& path)
{
	_standardPathResolver.pushRelativeSearchPath(path);
}

void Application::pushSearchPaths(const std::set<std::string>& paths)
{
	_standardPathResolver.pushSearchPaths(paths);
}

void Application::popSearchPaths(size_t amount)
{
	_standardPathResolver.popSearchPaths(amount);
}

void Application::setPathResolver(PathResolver::Pointer resolver)
{
	_customPathResolver = resolver;
}

void Application::setShouldSilentPathResolverErrors(bool e)
{
	_standardPathResolver.setSilentErrors(e);
}

const ApplicationIdentifier& Application::identifier() const
{
#if defined(ET_EMBEDDED_APPLICATION)
	static const ApplicationIdentifier dummy;
	return dummy;
#else
	return _identifier;
#endif
}


/*
 * Service
 */
RunLoop& et::currentRunLoop()
{
	if (Threading::currentThread() == application().backgroundThread().id())
		return application().backgroundRunLoop();
	
	return mainRunLoop();
}

TimerPool::Pointer et::currentTimerPool()
{
	return currentRunLoop().firstTimerPool();
}

const std::string et::kSystemEventType = "kSystemEventType";
const std::string et::kSystemEventRemoteNotification = "kSystemEventRemoteNotification";
const std::string et::kSystemEventRemoteNotificationStatusChanged = "kSystemEventRemoteNotificationStatusChanged";
const std::string et::kSystemEventOpenURL = "kSystemEventOpenURL";
