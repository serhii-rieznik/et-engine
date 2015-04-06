/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>

namespace et
{
	class ApplicationNotifier
	{
	public:
		void notifyLoaded()
			{ application().loaded(); }
		
		void notifyUpdate()
			{ application().performUpdateAndRender(); }
		
		void notifyActivated()
			{ application().setActive(true); }

		void notifyDeactivated()
			{ application().setActive(false); }

		void notifySuspended()
			{ application().suspend(); }

		void notifyResumed()
			{ application().resume(); }

		void notifyStopped()
			{ application().stop(); }
	
		void notifyTerminated()
			{ application().terminated(); }
		
		void notifyResize(const et::vec2i& sz)
			{ application().renderContext()->resized(sz); }
		
		bool shouldPerformRendering()
			{ return application().shouldPerformRendering(); }
		
		RenderContext* accessRenderContext()
			{ return application().renderContext(); }
	};
	
}
