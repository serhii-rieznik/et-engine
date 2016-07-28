/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>

#if (ET_PLATFORM_IOS)

#include <et/rendering/opengl/opengl.h>
#include <et/rendering/opengl/opengl_caps.h>
#include <et/rendering/interface/renderer.h>
#include <et/rendering/rendercontext.h>
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
	_params(params), _app(app)
{
	ET_PIMPL_INIT(RenderContext, params)
	
#if !defined(ET_EMBEDDED_APPLICATION)
	ET_ASSERT(sharedOpenGLViewController != nil);
#endif
	
	OpenGLCapabilities::instance().checkCaps();

	_renderState.setRenderContext(this);
	
	_programFactory = ProgramFactory::Pointer::create(this);
	_textureFactory = TextureFactory::Pointer::create(this);
	_vertexBufferFactory = VertexBufferFactory::Pointer::create(this);
	_renderer = Renderer::Pointer::create(this);
	
	CGSize screenSize = [[[UIApplication sharedApplication] delegate] window].frame.size;
	auto screenScale = [[UIScreen mainScreen] scale];
	
	vec2i contextSize(static_cast<int>(screenScale * screenSize.width),
		static_cast<int>(screenScale * screenSize.height));

	updateScreenScale(contextSize);
	_renderState.setMainViewportSize(contextSize);
}

RenderContext::~RenderContext()
{
	ET_PIMPL_FINALIZE(RenderContext)
}

void RenderContext::init()
{
#if !defined(ET_EMBEDDED_APPLICATION)
	[sharedOpenGLViewController setRenderContext:this];
#endif
	
	_fpsTimer.expired.connect(this, &RenderContext::onFPSTimerExpired);
	_fpsTimer.start(mainTimerPool(), 1.0f, NotifyTimer::RepeatForever);
}

size_t RenderContext::renderingContextHandle()
{
	return reinterpret_cast<size_t>(sharedOpenGLViewController);
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

void RenderContext::pushRenderingContext()
{
	// TODO
}

bool RenderContext::activateRenderingContext()
{
	// TODO
	return true;
}

bool RenderContext::pushAndActivateRenderingContext()
{
	pushRenderingContext();
	return activateRenderingContext();
}

void RenderContext::popRenderingContext()
{
	// TODO
}

/*
 * RenderContextPrivate
 */
RenderContextPrivate::RenderContextPrivate(const RenderContextParameters& params)
{
#if !defined(ET_EMBEDDED_APPLICATION)
	sharedOpenGLViewController = [[etOpenGLViewController alloc] initWithParameters:params];
#endif
}

#endif // ET_PLATFORM_IOS
