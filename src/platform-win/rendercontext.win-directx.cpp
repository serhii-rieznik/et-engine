/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_WIN)

#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")

#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et/input/input.h>

using namespace et;

LRESULT CALLBACK mainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct RenderContextData
{
	bool initialized = false;
	HWND hWnd = nullptr;
	HDC hDC = nullptr;

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* renderTarget = nullptr;

	RenderContextData& release();
};

class et::RenderContextPrivate : public Input::KeyboardInputSource, public Input::PointerInputSource
{
public:
	RenderContextPrivate(RenderContext* rc, RenderContextParameters& params, const ApplicationParameters& appParams);
	~RenderContextPrivate();

public:
	bool failed = false;

	HINSTANCE hInstance;
	WNDCLASSEXW wndClass;

	RenderContextData primaryContext;

	RenderContext* renderContext() 
		{ return _renderContext; }

	bool shouldPostMovementMessage(int x, int y);

private:
	HWND createWindow(size_t, WindowSize, vec2i&);

	bool initWindow(RenderContextParameters& params, const ApplicationParameters& appParams);
	bool initDirectX(const RenderContextParameters& params);

private:
	RenderContext* _renderContext = nullptr;
	int _mouseX = -1;
	int _mouseY = -1;
};

RenderContext::RenderContext(const RenderContextParameters& inParams, Application* app) : _params(inParams), _app(app),
	_programFactory(0), _textureFactory(0), _framebufferFactory(0), _vertexBufferFactory(0), _renderer(0)
{
	ET_PIMPL_INIT(RenderContext, this, _params, app->parameters())

	if (_private->failed)
	{
		ET_PIMPL_FINALIZE(RenderContext)
	}
	else 
	{
		_renderState.setRenderContext(this);
		_programFactory = ProgramFactory::Pointer::create(this);
		_textureFactory = TextureFactory::Pointer::create(this);
		_framebufferFactory = FramebufferFactory::Pointer::create(this);
		_vertexBufferFactory = VertexBufferFactory::Pointer::create(this);
		_renderer = Renderer::Pointer::create(this);

		_renderState.setDefaultFramebuffer(_framebufferFactory->createFramebufferWrapper(0, "default-fbo"));
		
		updateScreenScale(_params.contextSize);
	}
}

RenderContext::~RenderContext()
{
	_renderer.reset(nullptr);
	_vertexBufferFactory.reset(nullptr);
	_framebufferFactory.reset(nullptr);
	_textureFactory.reset(nullptr);
	_programFactory.reset(nullptr);

	ET_PIMPL_FINALIZE(RenderContext)
}

void RenderContext::init()
{
	RECT r = { };
	GetClientRect(_private->primaryContext.hWnd, &r);
	_renderState.setMainViewportSize(vec2i(r.right - r.left, r.bottom - r.top));

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
	vec4 clearColor(1.0f, 0.5f, 0.25f, 1.0f);
	_private->primaryContext.deviceContext->ClearRenderTargetView(_private->primaryContext.renderTarget, clearColor.data());
}

void RenderContext::endRender()
{
	_private->primaryContext.swapChain->Present(1, 0);
	++_info.averageFramePerSecond;
}

/*
 *
 * RenderContextPrivate
 * 
 */

RenderContextPrivate::RenderContextPrivate(RenderContext* rc, RenderContextParameters& params, 
	const ApplicationParameters& appParams) : failed(true), hInstance(nullptr), _renderContext(rc)
{
	if (initWindow(params, appParams))
	{
		if (initDirectX(params))
			failed = false;
	}
}

HWND RenderContextPrivate::createWindow(size_t style, WindowSize windowSize, vec2i& size)
{ 
	UINT windowStyle = WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX;

	if (windowSize != WindowSize_Fullscreen)
	{
		if ((style & WindowStyle_Caption) == WindowStyle_Caption)
			windowStyle |= WS_CAPTION | WS_ACTIVECAPTION;

		if ((style & WindowStyle_Sizable) == WindowStyle_Sizable)
			windowStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;

		if (windowSize == WindowSize_FillWorkarea)
			windowStyle |= WS_MAXIMIZE;
	}

	RECT windowRect = { };
	RECT workareaRect = { };
	SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&workareaRect, 0);
	vec2i workareaSize(workareaRect.right - workareaRect.left, workareaRect.bottom - workareaRect.top);

	if (windowSize == WindowSize_Fullscreen)
	{
		windowRect.right = GetSystemMetrics(SM_CXSCREEN);
		windowRect.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
	else if (windowSize == WindowSize_FillWorkarea)
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

	if (windowSize != WindowSize_Fullscreen)
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

	SetWindowLongPtr(primaryContext.hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	ShowWindow(primaryContext.hWnd, SW_SHOW);
	SetForegroundWindow(primaryContext.hWnd);
	SetFocus(primaryContext.hWnd);

	if (appParams.windowSize == WindowSize_Fullscreen)
	{
		DEVMODE dm = { };
		EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);
		ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
	}

	return true;
}

bool RenderContextPrivate::initDirectX(const RenderContextParameters& params)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { };
	swapChainDesc.BufferDesc.Width = params.contextSize.x;
	swapChainDesc.BufferDesc.Height = params.contextSize.y;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = primaryContext.hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = true;

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	size_t numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);
	size_t numFeatureLevels = sizeof(featureLevels) / sizeof(featureLevels[0]);

	HRESULT initializationResult = 0;
	for (size_t i = 0; i < numDriverTypes; ++i)
	{
		D3D_FEATURE_LEVEL selectedFeatureLevel = featureLevels[0];

		initializationResult = D3D11CreateDeviceAndSwapChain(nullptr, driverTypes[i], nullptr, 0, featureLevels,
			numFeatureLevels, D3D11_SDK_VERSION, &swapChainDesc, &primaryContext.swapChain,
			&primaryContext.device, &selectedFeatureLevel, &primaryContext.deviceContext);

		if (SUCCEEDED(initializationResult))
			break;
	}

	if (FAILED(initializationResult))
		return false;

	ID3D11Texture2D* backBuffer = nullptr;
	auto result = primaryContext.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(result) || (backBuffer == nullptr))
		return false;

	result = primaryContext.device->CreateRenderTargetView(backBuffer, nullptr, &primaryContext.renderTarget);
	backBuffer->Release();

	if (FAILED(result) || (primaryContext.renderTarget == nullptr))
		return false;

	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(params.contextSize.x);
	viewport.Height = static_cast<float>(params.contextSize.y);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	primaryContext.deviceContext->OMSetRenderTargets(1, &primaryContext.renderTarget, nullptr);
	primaryContext.deviceContext->RSSetViewports(1, &viewport);

	return true;
}

RenderContextPrivate::~RenderContextPrivate()
{
	primaryContext.release();
	UnregisterClassW(wndClass.lpszClassName, hInstance);
}

bool RenderContextPrivate::shouldPostMovementMessage(int x, int y)
{
	if ((x == _mouseX) && (y == _mouseY)) return false;

	_mouseX = x;
	_mouseY = y;
	return true;
}

/*
 * Render Context Data
 */ 

RenderContextData& RenderContextData::release()
{
	if (deviceContext)
		deviceContext->ClearState();

	if (renderTarget)
		renderTarget->Release();

	if (swapChain)
		swapChain->Release();

	if (deviceContext)
		deviceContext->Release();

	if (device)
		device->Release();

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

/*
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
	RenderContextPrivate* handler = (RenderContextPrivate*)GetWindowLong(hWnd, GWLP_USERDATA);
#else
	RenderContextPrivate* handler = (RenderContextPrivate*)GetWindowLong(hWnd, GWL_USERDATA);
#endif

	if (handler == 0)
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);

	ApplicationNotifier notifier;
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
			if (wParam == WA_INACTIVE)
				notifier.notifyDeactivated();
			else
				notifier.notifyActivated();

			return 0;
		}

	case WM_SIZE:
		{
			internal_SizeInfo size(lParam);
			notifier.notifyResize(vec2i(size.width, size.height));
			return 0;
		}

	default: 
		return DefWindowProcW(hWnd, uMsg, wParam, lParam); 
	}
}

#endif // ET_PLATFORM_WIN
