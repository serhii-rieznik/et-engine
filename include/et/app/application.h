/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/core/tools.h>
#include <et/app/runloop.h>
#include <et/app/appevironment.h>
#include <et/app/applicationdelegate.h>
#include <et/app/backgroundthread.h>
#include <et/rendering/rendercontext.h>

namespace et
{
	class ApplicationNotifier;
	class Application : public Singleton<Application>,  public EventReceiver
	{
	public:
		enum AlertType
		{
			AlertType_Information,
			AlertType_Warning,
			AlertType_Error
		};

	public: 
		int run(int argc, char* argv[]);
		void quit(int exitCode = 0);

		IApplicationDelegate* delegate();
		static IApplicationDelegate* initApplicationDelegate();

		RunLoop& mainRunLoop()
			{ return _runLoop; }

		RunLoop& backgroundRunLoop()
			{ return _backgroundThread.runLoop(); }
		
		size_t renderingContextHandle() const
			{ return _renderingContextHandle; }

		AppEnvironment& environment()
			{ return _env; }

		const ApplicationParameters& parameters() const
			{ return _parameters; }

		size_t launchParamtersCount() const
			{ return _launchParameters.size(); }

		const std::string& launchParameter(size_t i) const
			{ return (i >= _launchParameters.size()) ? _emptyParamter : _launchParameters.at(i); }

		const ApplicationIdentifier& identifier() const
			{ return _identifier; }

		bool running() const 
			{ return _running; }

		bool active() const 
			{ return _active; }

		bool suspended() const
			{ return _suspended; }

		size_t memoryUsage() const;
		size_t availableMemory() const;

		float cpuLoad() const;

		void setTitle(const std::string& s);
		void setFrameRateLimit(size_t value);

		void alert(const std::string& title, const std::string& message, AlertType type = AlertType_Information);
		
	private:
		friend class RenderContext;

		RenderContext* renderContext() 
			{ return _renderContext; }

		void performRendering();

		void setActive(bool active);

		void contextResized(const vec2i& size);

		void suspend();
		void resume();
		
		void stop();
		
		int platformRun(int, char* []);
		
		void platformInit();
		void platformFinalize();
		void platformActivate();
		void platformDeactivate();
		void platformSuspend();
		void platformResume();
		
		void terminated();
		
	private:
		friend class ApplicationNotifier;
		
		Application();
		~Application();

		ET_SINGLETON_COPY_DENY(Application)

		void enterRunLoop();
		void idle();
		void updateTimers(float dt);
		
		void loaded();

	private:
		static IApplicationDelegate* _delegate;
		
		ApplicationParameters _parameters;
		ApplicationIdentifier _identifier;
		RenderContext* _renderContext;
		AppEnvironment _env;
		RunLoop _runLoop;
		BackgroundThread _backgroundThread;

		std::string _emptyParamter;
		StringList _launchParameters;

		AtomicBool _running;
		AtomicBool _active;
		AtomicBool _suspended;
		
		size_t _renderingContextHandle;
		uint64_t _lastQueuedTimeMSec;
		uint64_t _fpsLimitMSec;
		uint64_t _fpsLimitMSecFractPart;
		
		int _exitCode;
		bool _postResizeOnActivate;
	};

	inline Application& application()
		{ return Application::instance(); }

	inline RunLoop& mainRunLoop()
		{ return Application::instance().mainRunLoop(); }

	inline RunLoop& backgroundRunLoop()
		{ return Application::instance().backgroundRunLoop(); }
	
	inline TimerPool::Pointer mainTimerPool()
		{ return Application::instance().mainRunLoop().mainTimerPool(); }
}
