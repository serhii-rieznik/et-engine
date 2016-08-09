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

#include <et/rendering/interface/renderer.h>

#include <et/rendering/renderingcaps.h>
#include <et/rendering/materialfactory.h>
#include <et/rendering/texturefactory.h>
#include <et/rendering/vertexbufferfactory.h>

namespace et
{
	class Application;
	class RenderContextPrivate;
	class RenderContext
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
			{ return _size; }

		RenderInterface::Pointer renderer()
			{ return _renderer; }

		void pushRenderingContext();
		bool activateRenderingContext();
		bool pushAndActivateRenderingContext();
		void popRenderingContext();

		bool beginRender();
		void endRender();
        
        void performResizing(const vec2i&);

	private:
		RenderContext(RenderContext&&) = delete;
		RenderContext(const RenderContext&) = delete;
		RenderContext& operator = (const RenderContext&) = delete;

	private:
		ET_DECLARE_PIMPL(RenderContext, 256)

		RenderContextParameters _params;
		Application* _app = nullptr;
		RenderInterface::Pointer _renderer;
		vec2i _size;
	};

}
