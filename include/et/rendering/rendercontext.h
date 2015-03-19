/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>
#include <et/core/objectscache.h>
#include <et/rendering/rendercontextparams.h>
#include <et/rendering/renderer.h>
#include <et/rendering/renderstate.h>
#include <et/rendering/renderingcaps.h>
#include <et/rendering/programfactory.h>
#include <et/rendering/texturefactory.h>
#include <et/rendering/framebufferfactory.h>
#include <et/rendering/vertexbufferfactory.h>
#include <et/timers/notifytimer.h>
#include <et/app/events.h>

namespace et
{
	class Application;
	class ApplicationNotifier;
	
	struct RenderingInfo
	{
		size_t averageFramePerSecond = 0;
		size_t averageDIPPerSecond = 0;
		size_t averagePolygonsPerSecond = 0;
		uint64_t averageFrameTimeInMicroseconds = 0;
	};

	class RenderContextPrivate;
	class RenderContextNotifier;
	
	class RenderContext : public EventReceiver
	{
	public:
		RenderContext(const RenderContextParameters& params, Application* app);
		~RenderContext();

		void init();
		bool valid();
		
		const RenderContextParameters& parameters() const
			{ return _params; }

		const vec2& size() const
			{ return _renderState.mainViewportSizeFloat(); }

		const vec2i& sizei() const
			{ return _renderState.mainViewportSize(); }

		size_t screenScaleFactor() const
			{ return _screenScaleFactor; }

		RenderState& renderState()
			{ return _renderState; }

		Renderer* renderer()
			{ return _renderer.ptr(); }

		ProgramFactory& programFactory()
			{ return _programFactory.reference(); }

		TextureFactory& textureFactory()
			{ return _textureFactory.reference(); }
		
		FramebufferFactory& framebufferFactory()
			{ return _framebufferFactory.reference(); }

		VertexBufferFactory& vertexBufferFactory()
			{ return _vertexBufferFactory.reference(); }

		size_t lastFPSValue() const
			{ return _info.averageFramePerSecond; }

		void beginRender();
		void endRender();

		size_t renderingContextHandle();

	public:
		ET_DECLARE_EVENT1(renderingInfoUpdated, const RenderingInfo&)
		ET_DECLARE_EVENT1(screenScaleFactorChanged, size_t)

	private:
		RenderContext(RenderContext&&) = delete;
		RenderContext(const RenderContext&) = delete;
		RenderContext& operator = (const RenderContext&) = delete;

		void onFPSTimerExpired(NotifyTimer* t);
		void resized(const vec2i&);
		void updateScreenScale(const vec2i& screenSize);

	private:
		friend class RenderContextPrivate;
		friend class RenderContextNotifier;
		friend class ApplicationNotifier;

		ET_DECLARE_PIMPL(RenderContext, 256)

		RenderContextParameters _params;

		Application* _app = nullptr;
		NotifyTimer _fpsTimer;
		RenderingInfo _info;

		RenderState _renderState;

		ProgramFactory::Pointer _programFactory;
		TextureFactory::Pointer _textureFactory;
		FramebufferFactory::Pointer _framebufferFactory;
		VertexBufferFactory::Pointer _vertexBufferFactory;
		Renderer::Pointer _renderer;
		
		size_t _screenScaleFactor = 1;
		size_t _maxScreenScaleFactor = 2;
		bool _screenScaleFactorSet = false;
	};

}
