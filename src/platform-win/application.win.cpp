/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/app/application.h>

#if (ET_PLATFORM_WIN)

#include <MMSystem.h>

using namespace et;

#if defined(_MSC_VER)
#	pragma comment(lib, "winmm.lib")
#	pragma comment(lib, "psapi.lib")
#endif

#define IF_CASE(A) case A: return #A;

std::string exceptionCodeToString(DWORD code)
{
	switch (code)
	{
		IF_CASE(EXCEPTION_ACCESS_VIOLATION)
		IF_CASE(EXCEPTION_DATATYPE_MISALIGNMENT)
		IF_CASE(EXCEPTION_BREAKPOINT)
		IF_CASE(EXCEPTION_SINGLE_STEP)
		IF_CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
		IF_CASE(EXCEPTION_FLT_DENORMAL_OPERAND)
		IF_CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO)
		IF_CASE(EXCEPTION_FLT_INEXACT_RESULT)
		IF_CASE(EXCEPTION_FLT_INVALID_OPERATION)
		IF_CASE(EXCEPTION_FLT_OVERFLOW)
		IF_CASE(EXCEPTION_FLT_STACK_CHECK)
		IF_CASE(EXCEPTION_FLT_UNDERFLOW)
		IF_CASE(EXCEPTION_INT_DIVIDE_BY_ZERO)
		IF_CASE(EXCEPTION_INT_OVERFLOW)
		IF_CASE(EXCEPTION_PRIV_INSTRUCTION)
		IF_CASE(EXCEPTION_IN_PAGE_ERROR)
		IF_CASE(EXCEPTION_ILLEGAL_INSTRUCTION)
		IF_CASE(EXCEPTION_NONCONTINUABLE_EXCEPTION)
		IF_CASE(EXCEPTION_STACK_OVERFLOW)
		IF_CASE(EXCEPTION_INVALID_DISPOSITION)
		IF_CASE(EXCEPTION_GUARD_PAGE)
		IF_CASE(EXCEPTION_INVALID_HANDLE)

	default:
		return "Unknown exception code: " + intToStr(code);
	}
}

LONG WINAPI unhandledExceptionFilter(struct _EXCEPTION_POINTERS* info)
{
	bool continuable = (info->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) == 0;
	
	void* backtrace[32] = { };
	DWORD backtraceHash = 0;
	WORD framesCaptured = RtlCaptureStackBackTrace(0, 32, backtrace, &backtraceHash);
	
	std::string excCode = exceptionCodeToString(info->ExceptionRecord->ExceptionCode);
	std::string type = continuable ? "continuable" : "non-continuable";
	log::info("Unhandled exception:\n code: %s\n type: %s\n address: 0x%08X", excCode.c_str(),
		type.c_str(), reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress));
	
	if (framesCaptured > 0)
	{
		log::info("Backtrace hash: 0x%08X", backtraceHash);
		for (int i = framesCaptured - 1; i >= 0; --i)
			log::info(" - 0x%08X", reinterpret_cast<uintptr_t>(backtrace[i]));
	}
	
	return EXCEPTION_EXECUTE_HANDLER;
}

void Application::platformInit()
{
#if (ET_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	SetUnhandledExceptionFilter(unhandledExceptionFilter);
	
	_env.updateDocumentsFolder(_identifier);
}

void Application::platformFinalize()
{
	sharedObjectFactory().deleteObject(_renderContext);
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

int Application::platformRun(int, char*[])
{
	RenderContextParameters params;
	delegate()->setRenderContextParameters(params); 

	_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
	_runLoop.updateTime(_lastQueuedTimeMSec);

	_renderContext = sharedObjectFactory().createObject<RenderContext>(params, this);
	if (_renderContext->valid())
	{
		_renderingContextHandle = _renderContext->renderingContextHandle();
		enterRunLoop();
		_delegate->applicationWillResizeContext(_renderContext->sizei());

		MSG msg = { };
		while (_running)
		{
			if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
			else if (shouldPerformRendering())
			{
				performUpdateAndRender();
			}
		}
	}

	terminated();

	sharedObjectFactory().deleteObject(_delegate);
	sharedObjectFactory().deleteObject(_renderContext);

	_delegate = nullptr;
	_renderContext = nullptr;

	return _exitCode;
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
	HWND window = reinterpret_cast<HWND>(_renderingContextHandle);
	SendMessage(window, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(s.c_str()));
}

void Application::requestUserAttention()
{
	FLASHWINFO fi = { };
	fi.cbSize = sizeof(fi);
	fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
	fi.hwnd = reinterpret_cast<HWND>(application().renderingContextHandle());
	fi.uCount = std::numeric_limits<UINT>::max();
	FlashWindowEx(&fi);
}

#endif //ET_PLATFORM_WIN
