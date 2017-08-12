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

struct ProfilerInfo
{
	uint64_t frameTime = 0;
};

class Application : public Singleton<Application>
{
public:
	int run(int argc, char* argv[]);
	void quit(int exitCode = 0);

	/*
	 * Extern methods, should be implemented by the client app
	 */
	IApplicationDelegate* initApplicationDelegate();
	const ApplicationIdentifier& identifier() const;

	IApplicationDelegate* delegate();
	RunLoop& mainRunLoop();
	BackgroundThread& backgroundThread();
	RunLoop& backgroundRunLoop();
	PlatformDependentContext& context();
	const PlatformDependentContext& context() const;
	Environment& environment();
	ApplicationParameters& parameters();
	const ApplicationParameters& parameters() const;
	const StringList& launchParameters() const;
	bool running() const;
	bool active() const;
	bool suspended() const;

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

	const ProfilerInfo& profiler() const 
		{ return _profiler; }

	ET_DECLARE_EVENT1(systemEvent, Dictionary);

public:
	void initContext();
	void freeContext();

	void suspend();
	void resume();
	void stop();

	void setActive(bool active);
	void resizeContext(const vec2i& size);

	bool shouldPerformRendering();
	void performUpdateAndRender();

private:
	friend class RenderContext;

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
	ProfilerInfo _profiler;

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

/*
 * Application's inline methods
 */
inline RunLoop& Application::mainRunLoop()
{
	return _runLoop;
}

inline BackgroundThread& Application::backgroundThread()
{
	return _backgroundThread;
}

inline RunLoop& Application::backgroundRunLoop()
{
	return _backgroundThread.runLoop();
}

inline PlatformDependentContext& Application::context()
{
	return _context;
}

inline const PlatformDependentContext& Application::context() const
{
	return _context;
}

inline Environment& Application::environment()
{
	return _env;
}

inline ApplicationParameters& Application::parameters()
{
	return _parameters;
}

inline const ApplicationParameters& Application::parameters() const
{
	return _parameters;
}

inline const StringList& Application::launchParameters() const
{
	return _launchParameters;
}

inline bool Application::running() const
{
	return _running;
}

inline bool Application::active() const
{
	return _active;
}

inline bool Application::suspended() const
{
	return _suspended;
}

}
