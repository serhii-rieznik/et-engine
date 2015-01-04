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
	class CriticalSectionPrivate;
	class CriticalSection
	{
	public:
		CriticalSection();
		~CriticalSection();

		void enter();
		void leave();

	private:
		ET_DECLARE_PIMPL(CriticalSection, 64);
	};

	class CriticalSectionScope
	{
	public:
		CriticalSectionScope(CriticalSection& section) :
			_cs(section) { _cs.enter(); }

		~CriticalSectionScope()
			{ _cs.leave(); }

	private:
		CriticalSectionScope& operator = (const CriticalSectionScope&) 
			{ return *this; }

	private:
		CriticalSection& _cs;
	};
}
