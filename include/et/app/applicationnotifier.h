/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
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
		
		void notifyIdle()
			{ application().idle(); }
		
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
		
		RenderContext* accessRenderContext()
			{ return application().renderContext(); }
	};
	
}
