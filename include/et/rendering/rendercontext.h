/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>
#include <et/core/objectscache.h>
#include <et/rendering/rendercontextparams.h>
#include <et/rendering/renderinterface.h>
#include <et/rendering/renderstate.h>
#include <et/rendering/renderingcaps.h>
#include <et/rendering/materialfactory.h>
#include <et/rendering/texturefactory.h>
#include <et/rendering/framebufferfactory.h>
#include <et/rendering/vertexbufferfactory.h>
#include <et/core/notifytimer.h>
#include <et/app/events.h>

namespace et
{
	class Application;
	
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
		void shutdown();
		
        RenderContextParameters& parameters()
            { return _params; }
        
        const RenderContextParameters& parameters() const
			{ return _params; }

		const vec2i& size() const
			{ return _renderState.mainViewportSize(); }

		RenderState& renderState()
			{ return _renderState; }

		RenderInterface* renderer()
			{ return _renderer.ptr(); }

		MaterialFactory& materialFactory()
			{ return _materialFactory.reference(); }

		TextureFactory& textureFactory()
			{ return _textureFactory.reference(); }
		
		FramebufferFactory& framebufferFactory()
			{ return _framebufferFactory.reference(); }

		VertexBufferFactory& vertexBufferFactory()
			{ return _vertexBufferFactory.reference(); }

		size_t lastFPSValue() const
			{ return _info.averageFramePerSecond; }

		void pushRenderingContext();
		bool activateRenderingContext();
		bool pushAndActivateRenderingContext();
		void popRenderingContext();

		bool beginRender();
		void endRender();
        
        void performResizing(const vec2i&);

	public:
		ET_DECLARE_EVENT1(renderingInfoUpdated, const RenderingInfo&)

	private:
		RenderContext(RenderContext&&) = delete;
		RenderContext(const RenderContext&) = delete;
		RenderContext& operator = (const RenderContext&) = delete;

		void onFPSTimerExpired(NotifyTimer* t);

	private:
		friend class RenderContextPrivate;
		friend class RenderContextNotifier;

		ET_DECLARE_PIMPL(RenderContext, 256)

		RenderContextParameters _params;

		Application* _app = nullptr;
		NotifyTimer _fpsTimer;
		RenderingInfo _info;

		RenderState _renderState;

		MaterialFactory::Pointer _materialFactory;
		TextureFactory::Pointer _textureFactory;
		FramebufferFactory::Pointer _framebufferFactory;
		VertexBufferFactory::Pointer _vertexBufferFactory;
		RenderInterface::Pointer _renderer;
	};

}
