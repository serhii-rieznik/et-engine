/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/core/tools.h>
#include <et/app/events.h>
#include <et/app/runloop.h>
#include <et/app/appevironment.h>
#include <et/app/applicationdelegate.h>
#include <et/app/backgroundthread.h>
#include <et/app/pathresolver.h>

namespace et
{
extern const std::string kSystemEventType;
extern const std::string kSystemEventRemoteNotification;
extern const std::string kSystemEventRemoteNotificationStatusChanged;
extern const std::string kSystemEventOpenURL;

class Application : public Singleton<Application>
{
public:
	int run(int argc, char* argv[]);

	void quit(int exitCode = 0);

	IApplicationDelegate* delegate();
	IApplicationDelegate* initApplicationDelegate();

	void initContext();
	void freeContext();

	RunLoop& mainRunLoop()
	{
		return _runLoop;
	}

	BackgroundThread& backgroundThread()
	{
		return _backgroundThread;
	}

	RunLoop& backgroundRunLoop()
	{
		return _backgroundThread.runLoop();
	}

	PlatformDependentContext& context()
	{
		return _context;
	}

	const PlatformDependentContext& context() const
	{
		return _context;
	}

	Environment& environment()
	{
		return _env;
	}

	ApplicationParameters& parameters()
	{
		return _parameters;
	}

	const ApplicationParameters& parameters() const
	{
		return _parameters;
	}

	const StringList& launchParameters() const
	{
		return _launchParameters;
	}

	const ApplicationIdentifier& identifier() const;

	bool running() const
	{
		return _running;
	}

	bool active() const
	{
		return _active;
	}

	bool suspended() const
	{
		return _suspended;
	}

	std::string resolveFileName(const std::string&);
	std::string resolveFolderName(const std::string&);
	std::set<std::string> resolveFolderNames(const std::string&);

	void pushSearchPath(const std::string&);
	void pushRelativeSearchPath(const std::string&);
	void pushSearchPaths(const std::set<std::string>&);
	void popSearchPaths(size_t = 1);
	void setShouldSilentPathResolverErrors(bool);

	void setPathResolver(PathResolver::Pointer);

	void setTitle(const std::string& s);
	void setFrameRateLimit(size_t value);

	void requestUserAttention();

	void enableRemoteNotifications();

	ET_DECLARE_EVENT1(systemEvent, Dictionary);

public:
	void load();
	void suspend();
	void resume();
	void stop();

	void setActive(bool active);
	void resizeContext(const vec2i& size);

	bool shouldPerformRendering();
	void performUpdateAndRender();

private:
	friend class RenderContext;

	RenderContext* renderContext()
	{
		return _renderContext;
	}

	int platformRun(int, char*[]);

	void platformInit();
	void platformFinalize();
	void platformActivate();
	void platformDeactivate();
	void platformSuspend();
	void platformResume();

	void enterRunLoop();
	void exitRunLoop();

private:
	Application();
	~Application();

	Application(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator = (const Application&) = delete;

	friend class et::Singleton<Application>;

private:
	ApplicationParameters _parameters;
	ApplicationIdentifier _identifier;

	RenderContext* _renderContext = nullptr;
	IApplicationDelegate* _delegate = nullptr;

	PlatformDependentContext _context;

	Environment _env;
	StandardPathResolver _standardPathResolver;
	PathResolver::Pointer _customPathResolver;

	RunLoop _runLoop;
	BackgroundThread _backgroundThread;

	std::string _emptyParamter;
	StringList _launchParameters;

	std::atomic<bool> _running{ false };
	std::atomic<bool> _active{ false };
	std::atomic<bool> _suspended{ false };

	uint64_t _lastQueuedTimeMSec = 0;
	uint64_t _fpsLimitMSec = 15;
	uint64_t _fpsLimitMSecFractPart = 0;

	int _exitCode = 0;
	vec2i _scheduledSize;
	bool _scheduleResize = false;
};

/*
 * currentRunLoop - returns background run loop if called in background and mainRunLoop otherwise
 */
Application& application();

RunLoop& mainRunLoop();
RunLoop& backgroundRunLoop();
RunLoop& currentRunLoop();

TimerPool::Pointer& mainTimerPool();
TimerPool::Pointer currentTimerPool();

void registerRunLoop(RunLoop&);
void unregisterRunLoop(RunLoop&);
}
