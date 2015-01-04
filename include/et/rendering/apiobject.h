/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	class APIObject : public LoadableObject
	{
	public:
		APIObject() :
			LoadableObject() { }
		
		APIObject(const std::string& aName) :
			LoadableObject(aName) { }
		
		APIObject(const std::string& aName, const std::string& aOrigin) :
			LoadableObject(aName, aOrigin)  { }
		
		size_t apiHandle() const
			{ return _apiHandle; }
		
		void setAPIHandle(size_t value)
			{ _apiHandle = value; }
		
		bool apiHandleValid() const
			{ return _apiHandle != 0; }

		bool apiHandleInvalid() const
			{ return _apiHandle == 0; }
		
	private:
		size_t _apiHandle = 0;
	};
}
