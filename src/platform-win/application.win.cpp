/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <Windows.h>
#include <Psapi.h>
#include <et/app/application.h>

using namespace et;

#if defined(_MSC_VER)
#	pragma comment(lib, "winmm.lib")
#	pragma comment(lib, "psapi.lib")
#endif

void Application::platformInit()
{
#if (ET_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	_env.updateDocumentsFolder(_identifier);
}

void Application::platformFinalize()
{
	delete _renderContext;
	_renderContext = nullptr;
}

void Application::platformSuspend()
{
}

void Application::platformResume()
{
}

void Application::platformActivate()
{
	timeBeginPeriod(1);
}

void Application::platformDeactivate()
{
	timeEndPeriod(1);
}

int Application::platformRun(int argc, char* argv[])
{
	RenderContextParameters params;
	delegate()->setRenderContextParameters(params); 

	_renderContext = new RenderContext(params, this);
	if (_renderContext->valid())
	{
		_active = true;

		_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
		_runLoop.update(_lastQueuedTimeMSec);

		_renderingContextHandle = _renderContext->renderingContextHandle();
		_renderContext->init();

		_delegate->applicationDidLoad(_renderContext);
		_delegate->applicationWillResizeContext(_renderContext->sizei());

		_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();

		enterRunLoop(); 
		terminated();

		delete _delegate, _delegate = nullptr;
		delete _renderContext, _renderContext = nullptr;
	}

	return _exitCode;
}

void Application::enterRunLoop()
{
	MSG msg = { };

	_running = true;
	while (_running)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			idle();
		}
	}
}

void Application::quit(int exitCode)
{
	_running = false;
	_exitCode = exitCode;
}

void Application::alert(const std::string& title, const std::string& message, AlertType type)
{
	UINT alType = MB_ICONINFORMATION;

	switch (type)
	{
	case AlertType_Warning: 
		{
			alType = MB_ICONWARNING;
			break;
		}

	case AlertType_Error: 
		{
			alType = MB_ICONERROR;
			break;
		}

	default:
		break;
	}

	MessageBox(0, message.c_str(), title.c_str(), alType);
}

void Application::setTitle(const std::string& s)
{
	SendMessage(reinterpret_cast<HWND>(_renderingContextHandle), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(s.c_str()));
}

size_t Application::memoryUsage() const
{
	PROCESS_MEMORY_COUNTERS pmc = { sizeof(PROCESS_MEMORY_COUNTERS) };
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, pmc.cb);
	return pmc.WorkingSetSize;
}