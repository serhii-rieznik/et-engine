/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/platform-android/nativeactivity.h>

#include <et/app/application.h>
#include <et/opengl/openglcaps.h>
#include <et/rendering/rendercontext.h>

using namespace et;

class et::RenderContextPrivate
{
public:
	RenderContextPrivate(RenderContext* rc);

public:
	RenderContext* renderContext;
	
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

	vec2i surfaceSize;
};

RenderContext::RenderContext(const RenderContextParameters& params, Application* app) : _params(params),
	_app(app), _programFactory(0), _textureFactory(0), _framebufferFactory(0), _vertexBufferFactory(0),
	_renderer(0), _screenScaleFactor(0)
{
	_private = new RenderContextPrivate(this);
	
	openGLCapabilites().checkCaps();
	updateScreenScale(_private->surfaceSize);
	
	_renderState.setRenderContext(this);
	_programFactory = ProgramFactory::Pointer(new ProgramFactory(this));
	_textureFactory = TextureFactory::Pointer(new TextureFactory(this));
	_framebufferFactory = FramebufferFactory::Pointer(new FramebufferFactory(this));
	_vertexBufferFactory = VertexBufferFactory::Pointer(new VertexBufferFactory(this));
	
	_renderState.setDefaultFramebuffer(_framebufferFactory->createFramebufferWrapper(0, "default-fbo"));
}

RenderContext::~RenderContext()
{
	delete _private;
}

void RenderContext::init()
{
	_renderer = Renderer::Pointer::create(this);
}

bool RenderContext::valid()
{
	return _private != nullptr;
}

size_t RenderContext::renderingContextHandle()
{
	return 0;
}

void RenderContext::beginRender()
{
	checkOpenGLError("RenderContext::beginRender");
	
	OpenGLCounters::reset();
	
	_renderState.bindDefaultFramebuffer();
}

void RenderContext::endRender()
{
	checkOpenGLError("RenderContext::endRender");
	
	_renderState.bindDefaultFramebuffer();
	eglSwapBuffers(_private->display, _private->surface);

	++_info.averageFramePerSecond;
	_info.averageDIPPerSecond += OpenGLCounters::DIPCounter;
	_info.averagePolygonsPerSecond += OpenGLCounters::primitiveCounter;
}

/**
 *
 * RenderContextPrivate
 *
 */

RenderContextPrivate::RenderContextPrivate(RenderContext* rc)
{
    const EGLint attribs[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_DEPTH_SIZE, 16,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_NONE
    };

	const EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLint format = 0;
    EGLint numConfigs = 0;
    EGLConfig config = 0;
	EGLint major = 0;
	EGLint minor = 0;

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	
	if (eglInitialize(display, &major, &minor) == EGL_FALSE)
	{
		ET_FAIL("eglInitialize failed.");
		return;
	}
	
	log::info("EGL initialised to version: %d.%d", major, minor);
	
	if (eglChooseConfig(display, attribs, &config, 1, &numConfigs) == EGL_FALSE)
	{
		ET_FAIL("eglChooseConfig failed.");
		return;
	}
	
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	android_app* sharedApplication = reinterpret_cast<android_app*>(application().renderingContextHandle());
    ANativeWindow_setBuffersGeometry(sharedApplication->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, sharedApplication->window, nullptr);
	if (surface == nullptr)
	{
		ET_FAIL("eglCreateWindowSurface failed.");
		return;
	}
	
    context = eglCreateContext(display, config, nullptr, contextAttribs);
	if (context == nullptr)
	{
		ET_FAIL("eglCreateContext failed.");
		return;
	}
	
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
	{
		ET_FAIL("eglMakeCurrent failed.");
		return;
	}
	
    eglQuerySurface(display, surface, EGL_WIDTH, &surfaceSize.x);
    eglQuerySurface(display, surface, EGL_HEIGHT, &surfaceSize.y);
	log::info("INITIALIZED: width = %d, height = %d", surfaceSize.x, surfaceSize.y);
}
