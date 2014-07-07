/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
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
		CriticalSectionPrivate* _private;
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
