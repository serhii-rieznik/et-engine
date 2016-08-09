/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>

#if (ET_PLATFORM_WIN)

#include <et/rendering/rendercontext.h>
#include <et/platform-win/context_win.h>
#include <ShellScalingAPI.h>
#include <MMSystem.h>
#include <DbgHelp.h>
	
#if defined(_MSC_VER)
#	pragma comment(lib, "winmm.lib")
#	pragma comment(lib, "psapi.lib")
#	pragma comment(lib, "Dbghelp.lib")
#endif

#define CASE_TO_STRING(A) case A: return #A;

namespace et
{

void enableDPIAwareness();
std::string exceptionCodeToString(DWORD);
LONG WINAPI unhandledExceptionFilter(struct _EXCEPTION_POINTERS* info);

void Application::platformInit()
{
#if (ET_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	enableDPIAwareness();
	SetUnhandledExceptionFilter(unhandledExceptionFilter);
	_env.updateDocumentsFolder(_identifier);
}

void Application::platformFinalize()
{
	if (_parameters.shouldPreserveRenderContext)
		_renderContext->pushAndActivateRenderingContext();

	_backgroundThread.stop();
	_backgroundThread.join();
	sharedObjectFactory().deleteObject(_delegate);

	if (_parameters.shouldPreserveRenderContext)
		_renderContext->popRenderingContext();

	sharedObjectFactory().deleteObject(_renderContext);

	_delegate = nullptr;
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

void Application::initContext()
{
	_context = createWindowsContextWithOptions(RenderingAPI::OpenGL, _parameters.context);
}

void Application::freeContext()
{
	destroyWindowsContext(_context);
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
		enterRunLoop();
		_delegate->applicationWillResizeContext(_renderContext->size());

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

		stop();
		platformFinalize();
	}

	return _exitCode;
}

void Application::quit(int exitCode)
{
	ET_ASSERT(_running);

	_running = false;
	_exitCode = exitCode;
}

Application::~Application()
{

}

void Application::setTitle(const std::string& s)
{
	auto stringValue = ET_STRING_TO_PARAM_TYPE(s);
	HWND window = reinterpret_cast<HWND>(_context.objects[0]);
	SendMessage(window, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(stringValue.c_str()));
}

void Application::requestUserAttention()
{
	FLASHWINFO fi = { };
	fi.cbSize = sizeof(fi);
	fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
	fi.hwnd = reinterpret_cast<HWND>(_context.objects[0]);
	fi.uCount = std::numeric_limits<UINT>::max();
	FlashWindowEx(&fi);
}

/*
 * Service functions
 */
std::string exceptionCodeToString(DWORD code)
{
    switch (code)
    {
        CASE_TO_STRING(EXCEPTION_ACCESS_VIOLATION)
        CASE_TO_STRING(EXCEPTION_DATATYPE_MISALIGNMENT)
        CASE_TO_STRING(EXCEPTION_BREAKPOINT)
        CASE_TO_STRING(EXCEPTION_SINGLE_STEP)
        CASE_TO_STRING(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
        CASE_TO_STRING(EXCEPTION_FLT_DENORMAL_OPERAND)
        CASE_TO_STRING(EXCEPTION_FLT_DIVIDE_BY_ZERO)
        CASE_TO_STRING(EXCEPTION_FLT_INEXACT_RESULT)
        CASE_TO_STRING(EXCEPTION_FLT_INVALID_OPERATION)
        CASE_TO_STRING(EXCEPTION_FLT_OVERFLOW)
        CASE_TO_STRING(EXCEPTION_FLT_STACK_CHECK)
        CASE_TO_STRING(EXCEPTION_FLT_UNDERFLOW)
        CASE_TO_STRING(EXCEPTION_INT_DIVIDE_BY_ZERO)
        CASE_TO_STRING(EXCEPTION_INT_OVERFLOW)
        CASE_TO_STRING(EXCEPTION_PRIV_INSTRUCTION)
        CASE_TO_STRING(EXCEPTION_IN_PAGE_ERROR)
        CASE_TO_STRING(EXCEPTION_ILLEGAL_INSTRUCTION)
        CASE_TO_STRING(EXCEPTION_NONCONTINUABLE_EXCEPTION)
        CASE_TO_STRING(EXCEPTION_STACK_OVERFLOW)
        CASE_TO_STRING(EXCEPTION_INVALID_DISPOSITION)
        CASE_TO_STRING(EXCEPTION_GUARD_PAGE)
        CASE_TO_STRING(EXCEPTION_INVALID_HANDLE)
    default:
        return "Unknown exception code: " + intToStr(code);
    }
}

LONG WINAPI unhandledExceptionFilter(struct _EXCEPTION_POINTERS* info)
{
    bool continuable = (info->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) == 0;
    
    auto process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    void* backtrace[32] = { };
    char symbolInfoData[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolInfoData);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    DWORD backtraceHash = 0;
    WORD framesCaptured = RtlCaptureStackBackTrace(0, 32, backtrace, &backtraceHash);
    
    std::string excCode = exceptionCodeToString(info->ExceptionRecord->ExceptionCode);
    std::string type = continuable ? "continuable" : "non-continuable";
    log::info("Unhandled exception:\n code: %s\n type: %s\n address: 0x%016llX", excCode.c_str(),
        type.c_str(), reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress));
    
    if (framesCaptured > 0)
    {
        log::info("Backtrace (hash = 0x%08X):", backtraceHash);
        for (unsigned int i = 0; i < framesCaptured; ++i)
        {
            SymFromAddr(process, reinterpret_cast<DWORD64>(backtrace[i]), 0, symbol);
            log::info("%u : %s (0x%016llX)", i, symbol->Name, symbol->Address);
        }
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}

using SetProcessDpiAwarenessFunc = HRESULT (*)(PROCESS_DPI_AWARENESS);

#ifndef DPI_ENUMS_DECLARED

enum
{
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
};

#endif

void enableDPIAwareness()
{
	auto shCore = LoadLibraryA("Shcore.dll");
	if (shCore)
	{
		SetProcessDpiAwarenessFunc setProcessDpiAwareness =
			(SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

		if (setProcessDpiAwareness != nullptr)
		{
			setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
		}

		FreeLibrary(shCore);
	}
}

}

#endif //ET_PLATFORM_WIN
