/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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

		virtual ~APIObjectFactory()
			{ }

		RenderContext* renderContext()
			{ return _rc; }
		
	protected:
		APIObjectFactory() :
			_rc(nullptr) { abort(); }
		
		ET_DENY_COPY(APIObjectFactory)
		
	private:
		RenderContext* _rc;
	};
}