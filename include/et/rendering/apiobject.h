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
	
	class APIObject : public LoadableObject
	{
	public:
		APIObject() :
			LoadableObject() { }
		
		APIObject(const std::string& aName) :
			LoadableObject(aName) { }
		
		APIObject(const std::string& aName, const std::string& aOrigin) :
			LoadableObject(aName, aOrigin)  { }
		
		uint32_t apiHandle() const
			{ return _apiHandle; }
		
		void setAPIHandle(uint32_t value)
			{ _apiHandle = value; }
		
		bool apiHandleValid() const
			{ return _apiHandle != 0; }

		bool apiHandleInvalid() const
			{ return _apiHandle == 0; }
		
	private:
		uint32_t _apiHandle = 0;
	};
	
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
