/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/context.h>

namespace et
{
	class RenderContext;
	struct RenderContextParameters;
	template <typename T> union vector2;
	typedef vector2<int32_t> vec2i;
	
	class IApplicationDelegate
	{
	public:
		virtual ~IApplicationDelegate() { }

		virtual et::ApplicationIdentifier applicationIdentifier() const = 0;

		virtual void setApplicationParameters(et::ApplicationParameters&) { }
		virtual void setRenderContextParameters(et::RenderContextParameters&) { }
		
		virtual void applicationDidLoad(et::RenderContext*) { }
		virtual void applicationWillActivate() { }
		virtual void applicationWillDeactivate() { }
		virtual void applicationWillSuspend() { }
		virtual void applicationWillResume() { }
		virtual void applicationWillTerminate() { }

		virtual void applicationWillResizeContext(const et::vec2i&) { }

		virtual void render(et::RenderContext*) { }
	};
}
