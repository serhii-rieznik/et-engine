/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	class RenderContext;

	class APIObjectFactory : public Shared
	{
	protected:
		APIObjectFactory(RenderContext* rc) :
			_rc(rc) { }

		virtual ~APIObjectFactory() { }

		RenderContext* renderContext()
			{ return _rc; }
		
	protected:
		APIObjectFactory() = delete;
		
		ET_DENY_COPY(APIObjectFactory)
		
	private:
		RenderContext* _rc;
	};
}
