/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

namespace et
{
#if (ET_PLATFORM_IOS || ET_PLATFORM_MAC || ET_PLATFORM_ANDROID)
	typedef int AtomicCounterType;
#elif (ET_PLATFORM_WIN)
	typedef long AtomicCounterType;
#else
#	error AtomicCounterType is not defined
#endif
	
	class AtomicCounter
	{
	public:
		AtomicCounter();
		
		AtomicCounterType retain();
		AtomicCounterType release();
		
		void setValue(AtomicCounterType);

		volatile const AtomicCounterType& atomicCounterValue() const
			{ return _counter; }

#if (ET_DEBUG)
		volatile bool notifyOnRetain;
		volatile bool notifyOnRelease;
#endif

	private:
		ET_DENY_COPY(AtomicCounter)
		
	private:
		volatile AtomicCounterType _counter;
	};
	
	class AtomicBool
	{
	public:
		AtomicBool();
		
		bool operator = (bool b);
		
		bool operator == (bool b);
		bool operator == (const AtomicBool&);

		bool operator != (bool b);
		bool operator != (const AtomicBool&);
		
		operator bool() const;
		
	private:
		volatile AtomicCounterType _value;
	};
}
