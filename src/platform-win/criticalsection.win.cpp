/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/threading/criticalsection.h>

#if (ET_PLATFORM_WIN)

#include <Windows.h>

namespace et
{
	class CriticalSectionPrivate
	{
	public:
		CriticalSectionPrivate() 
			{ InitializeCriticalSection(&_cs); }

		~CriticalSectionPrivate()
			{ DeleteCriticalSection(&_cs); }

		void enter()
			{ EnterCriticalSection(&_cs); }

		void leave()
			{ LeaveCriticalSection(&_cs); }

	private:
		RTL_CRITICAL_SECTION _cs;
	};
};

using namespace et;

CriticalSection::CriticalSection()
{
	ET_PIMPL_INIT(CriticalSection)
}

CriticalSection::~CriticalSection() 
{
	ET_PIMPL_FINALIZE(CriticalSection)
}

void CriticalSection::enter()
{
	_private->enter();
}

void CriticalSection::leave()
{
	_private->leave();
}

#endif // ET_PLATFORM_WIN
