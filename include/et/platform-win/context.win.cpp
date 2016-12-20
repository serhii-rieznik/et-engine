/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/platform-win/context.win.h>

#if (ET_PLATFORM_WIN)

#include <et/app/application.h>
#include <Windows.h>

namespace et
{

const wchar_t* windowClassName = L"et::window";

extern LRESULT WINAPI mainWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

void registerWindowClass()
{
	static bool classRegistered = false;

	if (classRegistered)
		return;

	WNDCLASSEX wndClass = { };
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_OWNDC;
	wndClass.lpfnWndProc = mainWindowProc;
	wndClass.hInstance = GetModuleHandle(nullptr);
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.lpszClassName = windowClassName;
	wndClass.hIcon = LoadIcon(wndClass.hInstance, MAKEINTRESOURCE(101));
	ATOM result = RegisterClassExW(&wndClass);

	classRegistered = result != 0;
}

PlatformDependentContext createWindowsContextWithOptions(ContextOptions& options)
{
	registerWindowClass();

	PlatformDependentContext result;

	UINT windowStyle = WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION;

	if (options.style & ContextOptions::Sizable)
	{
		windowStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;
	}

	RECT windowRect = {0, 0, options.size.x, options.size.y};
	RECT workareaRect = {};
	SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&workareaRect, 0);
	vec2i workareaSize(workareaRect.right - workareaRect.left, workareaRect.bottom - workareaRect.top);

	vec2i actualSize = options.size;
	if ((windowStyle & WS_CAPTION) == WS_CAPTION)
	{
		AdjustWindowRectEx(&windowRect, windowStyle, 0, WS_EX_APPWINDOW);
		actualSize = vec2i(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	}
	windowRect.left = workareaRect.left + (workareaSize.x - actualSize.x) / 2;
	windowRect.top = workareaRect.top + (workareaSize.y - actualSize.y) / 2;

	auto title = utf8ToUnicode(application().identifier().applicationName);

	HWND window = CreateWindowExW(WS_EX_APPWINDOW, windowClassName, title.c_str(), windowStyle,
		windowRect.left, windowRect.top, actualSize.x, actualSize.y, 0, 0, GetModuleHandle(nullptr), 0);

	HDC dc = GetDC(window);

	result.objects[0] = window;
	result.objects[1] = dc;
	
	return result;
}

void destroyWindowsContext(PlatformDependentContext context)
{
	// TODO
}

}

#endif
