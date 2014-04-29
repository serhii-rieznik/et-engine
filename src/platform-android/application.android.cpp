/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/platform-android/nativeactivity.h>
#include <et/geometry/geometry.h>
#include <et/input/input.h>
#include <et/app/application.h>
#include <et/app/applicationnotifier.h>

/**
 *
 * Native Activity entry point, handlers and extern declaration of "main" function
 *
 */

namespace et
{
	static android_app* _sharedApplication = nullptr;
	android_app* sharedAndroidApplication()
		{ return _sharedApplication; }

	static zip* _sharedZipArchive = nullptr;
	zip* sharedAndroidZipArchive()
		{ return _sharedZipArchive; }
}

using namespace et;

extern int main(int, char*[]);

void handleCommand(android_app* app, int32_t cmd);
int32_t handleInput(android_app* app, AInputEvent* event);

void android_main(android_app* state)
{
	app_dummy();

	_sharedApplication = state;
    _sharedApplication->onAppCmd = handleCommand;
    _sharedApplication->onInputEvent = handleInput;

	_sharedZipArchive = zip_open(applicationPackagePath().c_str(), 0, nullptr);

	application();
	main(0, 0);

	zip_close(_sharedZipArchive);
	exit(0);
}

static bool applicationLoaded = false;

static ApplicationNotifier sharedApplicationNotifier;
static Input::PointerInputSource sharedPointerInput;

#define THIS_CASE(A) case A: { log::info("handleCommand:" #A); break; }

void handleCommand(android_app* app, int32_t cmd)
{
    switch (cmd)
	{
		case APP_CMD_LOST_FOCUS:
		{
			if (applicationLoaded)
				sharedApplicationNotifier.notifyDeactivated();
			break;
		}

		case APP_CMD_GAINED_FOCUS:
		{
//			if (applicationLoaded)
//				sharedApplicationNotifier.notifyActivated();
			break;
		}
			
		case APP_CMD_PAUSE:
		{
			if (applicationLoaded)
				sharedApplicationNotifier.notifySuspended();
			
			break;
		}

		case APP_CMD_RESUME:
		{
//			if (applicationLoaded)
//				sharedApplicationNotifier.notifyResumed();
			break;
		}

		case APP_CMD_INIT_WINDOW:
		{
			sharedApplicationNotifier.notifyLoaded();
			break;
		}

		THIS_CASE(APP_CMD_START)
		THIS_CASE(APP_CMD_DESTROY)
		THIS_CASE(APP_CMD_INPUT_CHANGED)
		THIS_CASE(APP_CMD_TERM_WINDOW)
		THIS_CASE(APP_CMD_WINDOW_RESIZED)
		THIS_CASE(APP_CMD_WINDOW_REDRAW_NEEDED)
        THIS_CASE(APP_CMD_CONTENT_RECT_CHANGED)
        THIS_CASE(APP_CMD_CONFIG_CHANGED)
        THIS_CASE(APP_CMD_LOW_MEMORY)
        THIS_CASE(APP_CMD_SAVE_STATE)
		THIS_CASE(APP_CMD_STOP)
				  
		default:
			log::info("WARNING!!! handleCommand: ANOTHER COMMAND (%d)", cmd);
    }
}

PointerInputInfo pointerInfoFromEvent(AInputEvent* event, int index, const vec2& contextSize)
{
	int32_t pid = AMotionEvent_getPointerId(event, index);
	
	vec2 pos = floorv(vec2(AMotionEvent_getX(event, index), AMotionEvent_getY(event, index)));
	vec2 normalizedPos = vec2(2.0f, -2.0f) * pos / contextSize - vec2(1.0f, -1.0f);

	return PointerInputInfo(PointerType_General, pos, normalizedPos, vec2(0.0f), pid,
		queryContiniousTimeInSeconds(), PointerOrigin_Touchscreen);
}

int32_t handleMotionInput(android_app* app, AInputEvent* event)
{
	RenderContext* rc = sharedApplicationNotifier.accessRenderContext();
	if (rc == nullptr) return 1;
	
	int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
	if (action > AMOTION_EVENT_ACTION_POINTER_UP)
	{
		log::warning("Unsupported event action: %d", action);
		return 1;
	}
	
	int32_t src = AInputEvent_getSource(event);
	if (src != AINPUT_SOURCE_TOUCHSCREEN)
	{
		log::warning("Unsupported event source: %d", src);
		return 1;
	}
	
	static Input::PointerInputSource inputSource;
	
	int32_t numPointers = AMotionEvent_getPointerCount(event);
	for (int p = 0; p < numPointers; ++p)
	{
		PointerInputInfo info = pointerInfoFromEvent(event, p, rc->size());
		switch (action)
		{
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
			case AMOTION_EVENT_ACTION_DOWN:
			{
				inputSource.pointerPressed(info);
				return 1;
			};

			case AMOTION_EVENT_ACTION_MOVE:
			{
				inputSource.pointerMoved(info);
				return 1;
			};

			case AMOTION_EVENT_ACTION_POINTER_UP:
			case AMOTION_EVENT_ACTION_UP:
			{
				inputSource.pointerReleased(info);
				return 1;
			};

			case AMOTION_EVENT_ACTION_CANCEL:
			{
				inputSource.pointerCancelled(info);
				return 1;
			};

			case AMOTION_EVENT_ACTION_OUTSIDE:
				return 1;

			default:
				ET_FAIL("Invalid motion action.");
		}
	}

	return 1;
}

int32_t handleKeyInput(android_app* app, AInputEvent* event)
{
	int32_t action = AKeyEvent_getAction(event);
	int32_t keyCode = AKeyEvent_getKeyCode(event);

	if (action == AKEY_EVENT_ACTION_DOWN)
	{
		
	}
	else if (action == AKEY_EVENT_ACTION_UP)
	{
		if (keyCode == AKEYCODE_BACK)
			application().quit(0);
	}
	else if (action == AKEY_EVENT_ACTION_MULTIPLE)
	{
	}
	else
	{
		log::info("WARNING!!! AINPUT_EVENT_TYPE_KEY, action: %d", action);
	}

	return 1;
}

int32_t handleInput(android_app* app, AInputEvent* event)
{
	switch (AInputEvent_getType(event))
	{
		case AINPUT_EVENT_TYPE_KEY:
			return handleKeyInput(app, event);
			
		case AINPUT_EVENT_TYPE_MOTION:
			return handleMotionInput(app, event);
			
		default:
			return 0;
	}
}

void processEvents()
{
	int ident = 0;
	do
	{
		int events = 0;

		android_poll_source* source = nullptr;
		ident = ALooper_pollAll(0, nullptr, &events, (void**)(&source));

		if (ident >= 0)
			source->process(_sharedApplication, source);
	}
	while (ident >= 0);
}

void Application::loaded()
{
	log::info("Application::loaded()");

	_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
	_runLoop.update(_lastQueuedTimeMSec);
	
	RenderContextParameters parameters;
	delegate()->setRenderContextParameters(parameters);
	
	_renderContext = new RenderContext(parameters, this);
	_renderContext->init();

	_active = true;
	delegate()->applicationDidLoad(_renderContext);

	applicationLoaded = true;
	
	delegate()->applicationWillResizeContext(_renderContext->sizei());
}

void Application::enterRunLoop()
{
	log::info("Application::enterRunLoop()");
	ET_ASSERT(_sharedApplication != nullptr);

	_active = false;
	_running = true;
	
	while (_sharedApplication->destroyRequested == 0)
	{
		processEvents();
		
		if (_active)
			idle();
    }
}

void Application::quit(int exitCode)
{
	log::info("Application::quit()");
	
	ANativeActivity_finish(_sharedApplication->activity);
	_running = false;
}

void Application::setTitle(const std::string &s)
{
}

void Application::alert(const std::string&, const std::string&, AlertType)
{	
}

void Application::platformInit()
{
	log::info("Application::platformInit()");
	_env.updateDocumentsFolder(_identifier);
}

int Application::platformRun(int argc, char* argv[])
{
	_renderingContextHandle = reinterpret_cast<size_t>(_sharedApplication);
	
	log::info("Application::platformRun()");
	enterRunLoop();
	return 0;
}

void Application::platformFinalize()
{
	log::info("Application::platformFinalize()");
	delete _delegate, _delegate = nullptr;
	delete _renderContext, _renderContext = nullptr;
}

void Application::platformActivate()
{
	log::info("Application::platformActivate()");
	
	if (!_running)
		processEvents();
}

void Application::platformDeactivate()
{
	log::info("Application::platformDeactivate()");
}


void Application::platformSuspend()
{
	log::info("Application::platformSuspend()");
}

void Application::platformResume()
{
	log::info("Application::platformResume()");
}
