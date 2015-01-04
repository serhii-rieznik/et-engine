/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/rendering/rendercontext.h>
#include <et/app/application.h>
#include <et/rendering/renderer.h>
#include <et/opengl/openglcaps.h>

#if defined(ET_HAVE_COCOS)

#if !defined(ET_EMBEDDED_APPLICATION)
#
#	error Define ET_EMBEDDED_APPLICATION to compile
#
#endif

using namespace et;

class et::RenderContextPrivate
{
public:
	RenderContextPrivate(const RenderContextParameters& params);
	RenderContext* owner = nullptr;
	uint64_t frameDuration = 0;
};

RenderContext::RenderContext(const RenderContextParameters& params, Application* app) :
	_params(params), _app(app), _programFactory(nullptr), _textureFactory(nullptr), _framebufferFactory(nullptr),
	_vertexBufferFactory(nullptr), _renderer(nullptr)
{
	ET_PIMPL_INIT(RenderContext, params)
	
     openGLCapabilites().checkCaps();

	_renderState.setRenderContext(this);
	
	_programFactory = ProgramFactory::Pointer::create(this);
	_textureFactory = TextureFactory::Pointer::create(this);
	_framebufferFactory = FramebufferFactory::Pointer::create(this);
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
	
	_fpsTimer.expired.connect(this, &RenderContext::onFPSTimerExpired);
	_fpsTimer.start(mainTimerPool(), 1.0f, NotifyTimer::RepeatForever);
}

size_t RenderContext::renderingContextHandle()
{
	return 0;
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
	
}

#else
#	warning Define ET_HAVE_COCOS to compile
#endif
