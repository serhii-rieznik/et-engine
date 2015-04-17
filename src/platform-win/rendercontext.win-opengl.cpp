/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <stack>
#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/renderer.h>
#include <et/input/input.h>
#include <et/core/cout.h>

#if (ET_PLATFORM_WIN && !ET_DIRECTX_RENDER)

using namespace et;

LRESULT CALLBACK mainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct RenderContextData
{
	bool initialized = false;
	HWND hWnd = nullptr;
	HDC hDC = nullptr;
	HGLRC hGLRC = nullptr;
	RenderContextData& release();
};

class et::RenderContextPrivate : public Input::KeyboardInputSource, public Input::PointerInputSource
{
public:
	RenderContextPrivate(RenderContext* rc, RenderContextParameters& params, const ApplicationParameters& appParams);
	~RenderContextPrivate();

	void push();
	bool activate();
	bool pushAndActivate();
	void pop();

public:
	bool failed = true;

	HINSTANCE hInstance = nullptr;
	WNDCLASSEXW wndClass;
	RenderContextData primaryContext;
	RenderContextData dummyContext;

	RenderContext* renderContext() 
		{ return _renderContext; }

	bool shouldPostMovementMessage(int x, int y);

	std::stack<std::pair<HDC, HGLRC>> contextStack;

private:
	HWND createWindow(size_t, WindowSize, vec2i&);
	RenderContextData createDummyContext(HWND hWnd);

	bool initWindow(RenderContextParameters& params, const ApplicationParameters& appParams);
	bool initOpenGL(const RenderContextParameters& params);

	int chooseMSAAPixelFormat(HDC, PIXELFORMATDESCRIPTOR*);
	int chooseAAPixelFormat(HDC, PIXELFORMATDESCRIPTOR*);
	int choosePixelFormat(HDC, PIXELFORMATDESCRIPTOR*);

private:
	RenderContext* _renderContext = nullptr;
	int _mouseX = -1;
	int _mouseY = -1;
};

RenderContext::RenderContext(const RenderContextParameters& inParams, Application* app) : 
	_params(inParams), _app(app)
{
	ET_PIMPL_INIT(RenderContext, this, _params, app->parameters())

	if (app->parameters().shouldPreserveRenderContext)
		pushAndActivateRenderingContext();

	if (_private->failed)
	{
		ET_PIMPL_FINALIZE(RenderContext)
	}
	else 
	{
		OpenGLCapabilities::instance().checkCaps();

		_renderState.setRenderContext(this);
		_programFactory = ProgramFactory::Pointer::create(this);
		_textureFactory = TextureFactory::Pointer::create(this);
		_framebufferFactory = FramebufferFactory::Pointer::create(this);
		_vertexBufferFactory = VertexBufferFactory::Pointer::create(this);
		_renderer = Renderer::Pointer::create(this);

		_renderState.setDefaultFramebuffer(_framebufferFactory->createFramebufferWrapper(0, "default-fbo"));
		
		updateScreenScale(_params.contextSize);
	}

	if (app->parameters().shouldPreserveRenderContext)
		popRenderingContext();
}

RenderContext::~RenderContext()
{
	if (application().parameters().shouldPreserveRenderContext)
		pushAndActivateRenderingContext();

	_renderer.reset(nullptr);
	_vertexBufferFactory.reset(nullptr);
	_framebufferFactory.reset(nullptr);
	_textureFactory.reset(nullptr);
	_programFactory.reset(nullptr);

	if (application().parameters().shouldPreserveRenderContext)
		popRenderingContext();

	ET_PIMPL_FINALIZE(RenderContext)
}

void RenderContext::init()
{
	if (application().parameters().shouldCreateContext)
	{
		RECT r = {};
		GetClientRect(_private->primaryContext.hWnd, &r);
		_renderState.setMainViewportSize(vec2i(r.right - r.left, r.bottom - r.top));
	}

	_fpsTimer.expired.connect(this, &RenderContext::onFPSTimerExpired);
	_fpsTimer.start(mainTimerPool().ptr(), 1.0f, -1);
}

bool RenderContext::valid()
{
	return _private != nullptr;
}

size_t RenderContext::renderingContextHandle()
{
	return reinterpret_cast<size_t>(_private->primaryContext.hWnd);
}

void RenderContext::beginRender()
{
	OpenGLCounters::reset();

	if (application().parameters().shouldPreserveRenderContext)
		pushAndActivateRenderingContext();

	if (application().parameters().shouldCreateContext)
	{
		_renderState.bindDefaultFramebuffer();
		checkOpenGLError("RenderContext::beginRender");
	}
}

void RenderContext::endRender()
{
	if (application().parameters().shouldCreateContext)
	{
		checkOpenGLError("RenderContext::endRender");
		SwapBuffers(_private->primaryContext.hDC);
	}

	++_info.averageFramePerSecond;
	_info.averageDIPPerSecond += OpenGLCounters::DIPCounter;
	_info.averagePolygonsPerSecond += OpenGLCounters::primitiveCounter;

	if (application().parameters().shouldPreserveRenderContext)
		popRenderingContext();
}

void RenderContext::pushRenderingContext()
{
	_private->push();
}

bool RenderContext::activateRenderingContext()
{
	return _private->activate();
}

bool RenderContext::pushAndActivateRenderingContext()
{
	return _private->pushAndActivate();
}

void RenderContext::popRenderingContext()
{
	_private->pop();
}

/*
 *
 * RenderContextPrivate
 * 
 */

RenderContextPrivate::RenderContextPrivate(RenderContext* rc, RenderContextParameters& params, 
	const ApplicationParameters& appParams) : _renderContext(rc)
{
	if (application().parameters().shouldPreserveRenderContext)
		push();

	if (application().parameters().shouldCreateContext)
	{
		if (initWindow(params, appParams))
		{
			if (initOpenGL(params))
				failed = false;
		}
	}
	else 
	{
		failed = false;
	}

	if (!failed)
	{
		GLeeInit();
		checkOpenGLError("GLeeInit");

		if (application().parameters().shouldCreateContext && wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT(static_cast<int>(params.swapInterval));
			checkOpenGLError("RenderContextPrivate::initOpenGL -> wglSwapIntervalEXT");
		}
	}

	if (application().parameters().shouldPreserveRenderContext)
		pop();
}

HWND RenderContextPrivate::createWindow(size_t style, WindowSize windowSize, vec2i& size)
{ 
	UINT windowStyle = WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX;

	if (windowSize != WindowSize::Fullscreen)
	{
		if ((style & WindowStyle_Caption) == WindowStyle_Caption)
			windowStyle |= WS_CAPTION | WS_ACTIVECAPTION;

		if ((style & WindowStyle_Sizable) == WindowStyle_Sizable)
			windowStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;

		if (windowSize == WindowSize::FillWorkarea)
			windowStyle |= WS_MAXIMIZE;
	}

	RECT windowRect = { };
	RECT workareaRect = { };
	SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&workareaRect, 0);
	vec2i workareaSize(workareaRect.right - workareaRect.left, workareaRect.bottom - workareaRect.top);

	if (windowSize == WindowSize::Fullscreen)
	{
		windowRect.right = GetSystemMetrics(SM_CXSCREEN);
		windowRect.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
	else if (windowSize == WindowSize::FillWorkarea)
	{
		windowRect.right = workareaSize.x;
		windowRect.bottom = workareaSize.y;
	}
	else
	{
		windowRect.right = size.x;
		windowRect.bottom = size.y;
	}

	size = vec2i(windowRect.right, windowRect.bottom);
	vec2i actualSize = size;

	if (windowSize != WindowSize::Fullscreen)
	{
		if ((windowStyle & WS_CAPTION) == WS_CAPTION)
		{
			AdjustWindowRectEx(&windowRect, windowStyle, 0, WS_EX_APPWINDOW);
			actualSize = vec2i(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
		}
		windowRect.left = workareaRect.left + (workareaSize.x - windowRect.right) / 2;
		windowRect.top = workareaRect.top + (workareaSize.y - windowRect.bottom) / 2;
	}

	auto title = utf8ToUnicode(application().identifier().applicationName);

	HWND window = CreateWindowExW(WS_EX_APPWINDOW, wndClass.lpszClassName, title.c_str(), windowStyle, 
		windowRect.left, windowRect.top, actualSize.x, actualSize.y, 0, 0, hInstance, 0);

	GetClientRect(window, &windowRect);
	size = vec2i(windowRect.right, windowRect.bottom);

	return window;
}

bool RenderContextPrivate::initWindow(RenderContextParameters& params, const ApplicationParameters& appParams)
{
	hInstance = GetModuleHandle(0);

	memset(&wndClass, 0, sizeof(wndClass));
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_OWNDC;
	wndClass.lpfnWndProc = mainWndProc;
	wndClass.hInstance = hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.lpszClassName = L"etWindowClass";

	ATOM result = RegisterClassExW(&wndClass);
	ET_ASSERT(result);
	(void)result;

	primaryContext.hWnd = createWindow(appParams.windowStyle, appParams.windowSize, params.contextSize);
	ET_ASSERT(primaryContext.hWnd != nullptr);

#if (ET_PLATFORM_WIN64)
	SetWindowLongPtr(primaryContext.hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
#else
	SetWindowLong(primaryContext.hWnd, GWL_USERDATA, reinterpret_cast<LONG>(this));
#endif

	if ((appParams.windowStyle & WindowStyle_Hidden) == WindowStyle_Hidden)
	{
		ShowWindow(primaryContext.hWnd, SW_HIDE);
	}
	else 
	{
		ShowWindow(primaryContext.hWnd, SW_SHOW);
		SetForegroundWindow(primaryContext.hWnd);
		SetFocus(primaryContext.hWnd);
	}

	if (appParams.windowSize == WindowSize::Fullscreen)
	{
		DEVMODE dm = { };
		EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);
		ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
	}

	return true;
}

RenderContextData RenderContextPrivate::createDummyContext(HWND hWnd)
{
	RenderContextData result;

	result.hWnd = hWnd;
	result.hDC = GetDC(result.hWnd);
	if (result.hDC == 0) return result;

	PIXELFORMATDESCRIPTOR dummy_pfd = { };
	dummy_pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	dummy_pfd.nVersion = 1;
	dummy_pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	dummy_pfd.cColorBits = 24;
	dummy_pfd.cAlphaBits = 8;
	dummy_pfd.cDepthBits = 24;

	int nPixelFormat = ChoosePixelFormat(result.hDC, &dummy_pfd); 

	if (nPixelFormat == 0)
		return result.release();

	if (!SetPixelFormat(result.hDC, nPixelFormat, &dummy_pfd))
		return result.release();

	result.hGLRC = wglCreateContext(result.hDC);
	if (result.hGLRC == 0)
		return result.release();

	if (!wglMakeCurrent(result.hDC, result.hGLRC))
		return result.release();

	result.initialized = true;

	return result;
}

bool RenderContextPrivate::initOpenGL(const RenderContextParameters& params)
{
	vec2i dummySize;
	HWND dummyWindow = createWindow(WindowStyle_Borderless, WindowSize::Predefined, dummySize);

	if (dummyWindow == nullptr)
		return false;

	RenderContextData dummy = createDummyContext(dummyWindow);
	if (!dummy.initialized)	return false;

	auto local_wglCreateContextAttribsARB = (GLEEPFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	PIXELFORMATDESCRIPTOR pfd = { };
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SUPPORT_COMPOSITION;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;

	int pixelFormat = 0;

	if (params.multisamplingQuality != MultisamplingQuality_None)
	{
		pixelFormat = chooseAAPixelFormat(dummy.hDC, &pfd);

		if (!pixelFormat)
			pixelFormat = choosePixelFormat(dummy.hDC, &pfd);
	}
	else
	{
		pixelFormat = choosePixelFormat(dummy.hDC, &pfd);
	}

	dummy.release();
	DestroyWindow(dummyWindow);
	if (pixelFormat == 0) return false;

	primaryContext.hDC = GetDC(primaryContext.hWnd);
	if (primaryContext.hDC == 0) return false;

	if (!SetPixelFormat(primaryContext.hDC, pixelFormat, &pfd))
	{  
		pixelFormat = choosePixelFormat(primaryContext.hDC, &pfd);
		if (!SetPixelFormat(primaryContext.hDC, pixelFormat, &pfd))
		{
			primaryContext.release();
			return false;
		}
	}

	if ((params.contextTargetVersion.x < 3) || (local_wglCreateContextAttribsARB == nullptr))
	{
		log::warning("Using legacy methods to create OpenGL context.");

		primaryContext.hGLRC = wglCreateContext(primaryContext.hDC);
		if (primaryContext.hGLRC == 0)
		{
			primaryContext.release();
			return 0;
		}

		if (!renderContext()->activateRenderingContext())
		{
			primaryContext.release();
			return false;
		}
	} 
	else
	{
		int attrib_list[] = 
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, params.contextTargetVersion.x,

			WGL_CONTEXT_MINOR_VERSION_ARB, params.contextTargetVersion.y,

			WGL_CONTEXT_FLAGS_ARB, 
				params.debugContext ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,

			WGL_CONTEXT_PROFILE_MASK_ARB, 
				params.compatibilityProfile ? WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB : 
					WGL_CONTEXT_CORE_PROFILE_BIT_ARB,

			0, 0
		};

		primaryContext.hGLRC = local_wglCreateContextAttribsARB(primaryContext.hDC, 0, attrib_list);
		if (primaryContext.hGLRC == 0)
		{
			DWORD lastError = GetLastError() & 0xffff;

			if (lastError == ERROR_INVALID_VERSION_ARB)
			{
				log::warning("Attempt to create OpenGL context with version %d.%d failed, trying previous versions...",
					attrib_list[1], attrib_list[3]);
			}
			else if (lastError == ERROR_INVALID_PROFILE_ARB)
				log::error("Error creating context: ERROR_INVALID_PROFILE_ARB");
			else if (lastError == ERROR_INVALID_PIXEL_FORMAT)
				log::error("Error creating context: ERROR_INVALID_PIXEL_FORMAT");
		}

		while (primaryContext.hGLRC == 0)
		{
			if (attrib_list[3] == 0)
			{
				attrib_list[1] -= 1;
				attrib_list[3] = 9;
			}
			else
			{
				attrib_list[3] -= 1;
			}

			if (attrib_list[1] < 2)
			{
				primaryContext.release();
				return false;
			}

			primaryContext.hGLRC = local_wglCreateContextAttribsARB(primaryContext.hDC, 0, attrib_list);
			if (primaryContext.hGLRC == 0)
			{
				DWORD lastError = GetLastError();
				if (lastError == ERROR_INVALID_VERSION_ARB)
				{
					log::warning("Attempt to create OpenGL context with version %d.%d failed, trying previous versions...",
						attrib_list[1], attrib_list[3]);
				}
				else if (lastError == ERROR_INVALID_PROFILE_ARB)
					log::error("Error creating context: ERROR_INVALID_PROFILE_ARB");
			}
		}

		if (!activate())
		{
			primaryContext.release();
			return 0;
		}
	}

	return true;
}

int RenderContextPrivate::chooseMSAAPixelFormat(HDC aDC, PIXELFORMATDESCRIPTOR*)
{
	int returnedPixelFormat = 0;

	auto local_wglChoosePixelFormatARB = (GLEEPFNWGLCHOOSEPIXELFORMATARBPROC)(wglGetProcAddress("wglChoosePixelFormatARB"));

	int attributes[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,     24,
		WGL_ALPHA_BITS_ARB,     8,
		WGL_DEPTH_BITS_ARB,     24,
		WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB,        0,
		0, 0
	};

	int maxSamples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

	UINT numFormats = 0;
	BOOL bStatus = FALSE;

	for (; maxSamples > 0; maxSamples /= 2)
	{
		attributes[17] = maxSamples;
		bStatus = local_wglChoosePixelFormatARB(aDC, attributes, 0, 1, &returnedPixelFormat, &numFormats);
		if (bStatus && numFormats) break;
	}

	return returnedPixelFormat;
}

int RenderContextPrivate::chooseAAPixelFormat(HDC aDC, PIXELFORMATDESCRIPTOR* pfd)
{
	return chooseMSAAPixelFormat(aDC, pfd);
}

int RenderContextPrivate::choosePixelFormat(HDC aDC, PIXELFORMATDESCRIPTOR* pfd)
{
	return ChoosePixelFormat(aDC, pfd);
}

RenderContextPrivate::~RenderContextPrivate()
{
	if (application().parameters().shouldCreateContext)
	{
		if (application().parameters().shouldPreserveRenderContext)
			renderContext()->pushAndActivateRenderingContext();

		primaryContext.release();
		UnregisterClassW(wndClass.lpszClassName, hInstance);

		if (application().parameters().shouldPreserveRenderContext)
			renderContext()->popRenderingContext();
	}
}

bool RenderContextPrivate::shouldPostMovementMessage(int x, int y)
{
	if ((x == _mouseX) && (y == _mouseY)) return false;

	_mouseX = x;
	_mouseY = y;
	return true;
}

void RenderContextPrivate::push()
{
	auto currentDC = wglGetCurrentDC();
	auto currentRC = wglGetCurrentContext();
	contextStack.push(std::make_pair(currentDC, currentRC));
}

bool RenderContextPrivate::activate()
{
	return wglMakeCurrent(primaryContext.hDC, primaryContext.hGLRC) == TRUE;
}

bool RenderContextPrivate::pushAndActivate()
{
	push();
	return activate();
}

void RenderContextPrivate::pop()
{
	ET_ASSERT(!contextStack.empty());

	auto top = contextStack.top();
	wglMakeCurrent(top.first, top.second);
}

/*
 * Render Context Data
 */ 

RenderContextData& RenderContextData::release()
{
	if (hGLRC != 0)
	{
		if ((hDC != 0) && (wglGetCurrentContext() == hGLRC))
			wglMakeCurrent(hDC, 0);

		wglDeleteContext(hGLRC);
		hGLRC = 0;
	}

	if ((hWnd != 0) && (hDC != 0))
	{
		ReleaseDC(hWnd, hDC);
		hDC = 0;
	}

	if (hWnd != 0)
	{
		DestroyWindow(hWnd);
		hWnd = 0;
	}

	initialized = false;
	return *this;
}

/**
 * Primary window proc
 */

union internal_PointerInputInfo
{
	LPARAM lParam;
	struct 
	{
		short x;
		short y; 
	};

	internal_PointerInputInfo(LPARAM p) : 
		lParam(p) { }
};

union internal_KeyInputInfo
{
	WPARAM wParam;
	struct { unsigned char code, unused1, unused2, unused3; };

	internal_KeyInputInfo(WPARAM p) : 
		wParam(p) { }
};

union internal_SizeInfo
{
	LPARAM lParam;
	struct { short width, height; };

	internal_SizeInfo(LPARAM p) : lParam(p) { };
};

static int mouseCaptures = 0;

void captureMouse(HWND wnd)
{
	++mouseCaptures;
	SetCapture(wnd);
}

void releaseMouse()
{
	--mouseCaptures;
	if (mouseCaptures <= 0)
	{
		mouseCaptures = 0;
		ReleaseCapture();
	}
}

LRESULT CALLBACK mainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if ET_PLATFORM_WIN64
	RenderContextPrivate* handler = (RenderContextPrivate*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
#else
	RenderContextPrivate* handler = (RenderContextPrivate*)GetWindowLong(hWnd, GWL_USERDATA);
#endif

	if (handler == nullptr)
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);

	vec2 viewportSize = handler->renderContext()->size();

	switch (uMsg)
	{ 
	case WM_QUIT:
	case WM_CLOSE:
		{
			application().quit();
			return 0;
		}

	case WM_LBUTTONDOWN:
		{
			captureMouse(hWnd);
			internal_PointerInputInfo p(lParam);

			vec2 pt(static_cast<float>(p.x), static_cast<float>(p.y));
			vec2 normPt(2.0f * pt.x / viewportSize.x - 1.0f, 1.0f - 2.0f * pt.y / viewportSize.y);

			handler->pointerPressed(PointerInputInfo(PointerType_General, pt, normPt, vec2(0.0f), 
				PointerType_General, mainRunLoop().time(), PointerOrigin_Mouse));
			return 0;
		}

	case WM_LBUTTONUP:
		{
			releaseMouse();
			internal_PointerInputInfo p(lParam);

			vec2 pt(static_cast<float>(p.x), static_cast<float>(p.y));
			vec2 normPt(2.0f * pt.x / viewportSize.x - 1.0f, 1.0f - 2.0f * pt.y / viewportSize.y);

			handler->pointerReleased(PointerInputInfo(PointerType_General, pt, normPt, vec2(0.0f), 
				PointerType_General, mainRunLoop().time(), PointerOrigin_Mouse));
			return 0;
		}

	case WM_RBUTTONDOWN:
		{
			captureMouse(hWnd);
			internal_PointerInputInfo p(lParam);

			vec2 pt(static_cast<float>(p.x), static_cast<float>(p.y));
			vec2 normPt(2.0f * pt.x / viewportSize.x - 1.0f, 1.0f - 2.0f * pt.y / viewportSize.y);

			handler->pointerPressed(PointerInputInfo(PointerType_RightButton, pt, normPt, vec2(0.0f), 
				PointerType_RightButton, mainRunLoop().time(), PointerOrigin_Mouse));

			return 0;
		}

	case WM_RBUTTONUP:
		{
			releaseMouse();
			internal_PointerInputInfo p(lParam);
			vec2 pt(static_cast<float>(p.x), static_cast<float>(p.y));
			vec2 normPt(2.0f * pt.x / viewportSize.x - 1.0f, 1.0f - 2.0f * pt.y / viewportSize.y);
			handler->pointerReleased(PointerInputInfo(PointerType_RightButton, pt, normPt, vec2(0.0f), 
				PointerType_RightButton, mainRunLoop().time(), PointerOrigin_Mouse));
			return 0;
		}

	case WM_MBUTTONDOWN:
		{
			captureMouse(hWnd);
			internal_PointerInputInfo p(lParam);
			vec2 pt(static_cast<float>(p.x), static_cast<float>(p.y));
			vec2 normPt(2.0f * pt.x / viewportSize.x - 1.0f, 1.0f - 2.0f * pt.y / viewportSize.y);
			handler->pointerPressed(PointerInputInfo(PointerType_MiddleButton, pt, normPt, vec2(0.0f), 
				PointerType_MiddleButton, mainRunLoop().time(), PointerOrigin_Mouse));
			return 0;
		}

	case WM_MBUTTONUP:
		{
			releaseMouse();
			internal_PointerInputInfo p(lParam);
			vec2 pt(static_cast<float>(p.x), static_cast<float>(p.y));
			vec2 normPt(2.0f * pt.x / viewportSize.x - 1.0f, 1.0f - 2.0f * pt.y / viewportSize.y);
			handler->pointerReleased(PointerInputInfo(PointerType_MiddleButton, pt, normPt, vec2(0.0f),
				PointerType_MiddleButton, mainRunLoop().time(), PointerOrigin_Mouse));
			return 0;
		}

	case WM_KEYDOWN:
		{
			internal_KeyInputInfo k(wParam);
			handler->keyPressed(k.code);
			return 0;
		}

	case WM_KEYUP:
		{
			internal_KeyInputInfo k(wParam);
			handler->keyReleased(k.code);
			return 0;
		}

	case WM_UNICHAR:
	{
		if (wParam >= ET_SPACE)
		{
			wchar_t wChars[2] = { static_cast<wchar_t>(wParam), 0 };
			handler->charactersEntered(unicodeToUtf8(wChars));
		}
		return 0;
	}

	case WM_CHAR:
		{
			if (wParam >= ET_SPACE)
			{
				wchar_t wChars[2] = { static_cast<wchar_t>(wParam), 0 };
				handler->charactersEntered(unicodeToUtf8(wChars));
			}
			return 0;
		}

	case WM_MOUSEMOVE:
		{
			internal_PointerInputInfo p(lParam);
			if (handler->shouldPostMovementMessage(p.x, p.y))
			{
				float t = mainRunLoop().time();
				vec2 pt(static_cast<float>(p.x), static_cast<float>(p.y));
				vec2 normPt(2.0f * pt.x / viewportSize.x - 1.0f, 1.0f - 2.0f * pt.y / viewportSize.y);

				PointerType pointer = PointerType_None;
				pointer += (wParam & MK_LBUTTON) == MK_LBUTTON ? PointerType_General : 0;
				pointer += (wParam & MK_RBUTTON) == MK_RBUTTON ? PointerType_RightButton : 0;
				pointer += (wParam & MK_MBUTTON) == MK_MBUTTON ? PointerType_MiddleButton : 0;

				handler->pointerMoved(PointerInputInfo(pointer, pt, normPt, vec2(0.0f), pointer, t, PointerOrigin_Mouse));
			}
			return 0;
		}

	case WM_MOUSEWHEEL:
		{
			internal_PointerInputInfo p(lParam);
			float s = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);

			POINT aPoint = { p.x, p.y };
			ScreenToClient(hWnd, &aPoint);

			vec2 pt(static_cast<float>(aPoint.x), static_cast<float>(aPoint.y));
			vec2 normPt(2.0f * pt.x / viewportSize.x - 1.0f, 1.0f - 2.0f * pt.y / viewportSize.y);

			handler->pointerScrolled(PointerInputInfo(PointerType_MiddleButton, pt, normPt, vec2(s),
				PointerType_MiddleButton, mainRunLoop().time(), PointerOrigin_Mouse));

			return 0;
		}

	case WM_ACTIVATE:
		{
			ApplicationNotifier notifier;

			if (wParam == WA_INACTIVE)
				notifier.notifyDeactivated();
			else
				notifier.notifyActivated();

			return 0;
		}

	case WM_SIZE:
		{
			internal_SizeInfo size(lParam);
			vec2i newSize(size.width, size.height);

			ApplicationNotifier notifier;
			notifier.accessRenderContext()->renderState().defaultFramebuffer()->resize(newSize);
			notifier.notifyResize(newSize);

			return 0;
		}

	default: 
		return DefWindowProcW(hWnd, uMsg, wParam, lParam); 
	}
}

#endif // ET_PLATFORM_WIN && !ET_DIRECTX_RENDER
