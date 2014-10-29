/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

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
