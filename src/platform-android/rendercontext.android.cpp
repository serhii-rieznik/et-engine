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
	RenderContextPrivate();

public:
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

	vec2i surfaceSize;
};

RenderContext::RenderContext(const RenderContextParameters& params, Application* app) : _params(params),
	_app(app), _programFactory(0), _textureFactory(0), _framebufferFactory(0), _vertexBufferFactory(0),
	_renderer(0), _screenScaleFactor(0)
{
	_private = new RenderContextPrivate();
	openGLCapabilites().checkCaps();

	updateScreenScale(_private->surfaceSize);
	
	_renderState.setRenderContext(this);
	_programFactory = new ProgramFactory(this);
	_textureFactory = new TextureFactory(this);
	_framebufferFactory = new FramebufferFactory(this, _textureFactory.ptr());
	_vertexBufferFactory = new VertexBufferFactory(_renderState);
}

RenderContext::~RenderContext()
{
	delete _private;
}

void RenderContext::init()
{
	_renderer = new Renderer(this);
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
	OpenGLCounters::reset();
	checkOpenGLError("RenderContext::beginRender");
}

void RenderContext::endRender()
{
	checkOpenGLError("RenderContext::endRender");
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

RenderContextPrivate::RenderContextPrivate()
{
    const EGLint attribs[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
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

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	
    eglInitialize(display, 0, 0);
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	android_app* sharedApplication = reinterpret_cast<android_app*>(application().renderingContextHandle());
    ANativeWindow_setBuffersGeometry(sharedApplication->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, sharedApplication->window, NULL);
    context = eglCreateContext(display, config, NULL, contextAttribs);
	
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) return;

    eglQuerySurface(display, surface, EGL_WIDTH, &surfaceSize.x);
    eglQuerySurface(display, surface, EGL_HEIGHT, &surfaceSize.y);

	log::info("INITIALIZED: width = %d, height = %d", surfaceSize.x, surfaceSize.y);
}