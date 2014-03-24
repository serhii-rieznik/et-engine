/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/opengl/opengl.h>
#include <et/rendering/rendercontext.h>
#include <et/app/application.h>
#include <et/rendering/renderer.h>
#include <et/opengl/openglcaps.h>
#include <et/platform-ios/openglviewcontroller.h>

using namespace et;

etOpenGLViewController* sharedOpenGLViewController = nil;

class et::RenderContextPrivate
{
public:
	RenderContextPrivate(const RenderContextParameters& params);
	
	RenderContext* owner = nullptr;
	uint64_t frameDuration = 0;
};

RenderContext::RenderContext(const RenderContextParameters& params, Application* app) :
	_params(params), _app(app), _programFactory(nullptr), _textureFactory(nullptr), _framebufferFactory(nullptr),
	_vertexBufferFactory(nullptr), _renderer(nullptr), _private(new RenderContextPrivate(params))
{
	ET_ASSERT(sharedOpenGLViewController != nil);
	
     openGLCapabilites().checkCaps();

	_renderState.setRenderContext(this);
	
	_programFactory = ProgramFactory::Pointer(new ProgramFactory(this));
	_textureFactory = TextureFactory::Pointer(new TextureFactory(this));
	_framebufferFactory = FramebufferFactory::Pointer(new FramebufferFactory(this));
	_vertexBufferFactory = VertexBufferFactory::Pointer(new VertexBufferFactory(this));
	
	CGSize screenSize = [[[UIApplication sharedApplication] delegate] window].frame.size;
	float screenScale = [[UIScreen mainScreen] scale];
	
	vec2i contextSize(static_cast<int>(screenScale * screenSize.width),
		static_cast<int>(screenScale * screenSize.height));

	updateScreenScale(contextSize);
	_renderState.setMainViewportSize(contextSize);
}

RenderContext::~RenderContext()
{
	delete _private;
	_private = nullptr;
}

void RenderContext::init()
{
	_renderer = Renderer::Pointer::create(this);
	
#if !defined(ET_EMBEDDED_APPLICATION)	
	[sharedOpenGLViewController setRenderContext:this];
#endif
	
	_fpsTimer.expired.connect(this, &RenderContext::onFPSTimerExpired);
	_fpsTimer.start(mainTimerPool(), 1.0f, NotifyTimer::RepeatForever);
}

size_t RenderContext::renderingContextHandle()
{
#if defined(ET_EMBEDDED_APPLICATION)	
	return 0;
#else	
	return reinterpret_cast<size_t>(sharedOpenGLViewController);
#endif	
}

void RenderContext::beginRender()
{
	OpenGLCounters::reset();
	_private->frameDuration = queryCurrentTimeInMicroSeconds();
}

void RenderContext::endRender()
{
	_info.averageDIPPerSecond += OpenGLCounters::DIPCounter;
	_info.averagePolygonsPerSecond += OpenGLCounters::primitiveCounter;
	
	++_info.averageFramePerSecond;
	_info.averageFrameTimeInMicroseconds += queryCurrentTimeInMicroSeconds() - _private->frameDuration;
}

/*
 * RenderContextPrivate
 */
RenderContextPrivate::RenderContextPrivate(const RenderContextParameters& params)
{
	sharedOpenGLViewController = [[etOpenGLViewController alloc] initWithParameters:params];
}
