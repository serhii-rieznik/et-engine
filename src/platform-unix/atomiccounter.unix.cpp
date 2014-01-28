/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */


#include <et/core/et.h>

#if (ET_PLATFORM_ANDROID)
#	include <sys/atomics.h>
#else
#	include <libkern/OSAtomic.h>
#endif

using namespace et;

#if ET_DEBUG

	static const AtomicCounterType validMask =
		static_cast<AtomicCounterType>(0xfffffffc);

#endif

AtomicCounter::AtomicCounter() :
	_counter(0)
{
}

AtomicCounterType AtomicCounter::retain()
{
#if (ET_PLATFORM_ANDROID)
	return __atomic_inc(&_counter);
#else
	return OSAtomicIncrement32(&_counter);
#endif
}

AtomicCounterType AtomicCounter::release()
{
#if (ET_PLATFORM_ANDROID)
	return __atomic_dec(&_counter);
#else
	return OSAtomicDecrement32(&_counter);
#endif
}

void AtomicCounter::setValue(AtomicCounterType value)
{
#if (ET_PLATFORM_APPLE)
	OSAtomicCompareAndSwap32(_counter, value, &_counter);
#else
	__sync_val_compare_and_swap(&_counter, _counter, value);
#endif
	
}

AtomicBool::AtomicBool() :
	_value(0) { }

bool AtomicBool::operator = (bool b)
{
	ET_ASSERT((_value & validMask) == 0);
#if (ET_PLATFORM_ANDROID)
	__atomic_swap(b, &_value);
#else
	OSAtomicCompareAndSwap32Barrier(_value, AtomicCounterType(b), &_value);
#endif
	return (_value != 0);
}

bool AtomicBool::operator == (bool b)
	{ ET_ASSERT((_value & validMask) == 0); return b == (_value != 0); }

bool AtomicBool::operator == (const AtomicBool& r)
	{ ET_ASSERT((_value & validMask) == 0); return (r._value != 0) == (_value != 0); }

bool AtomicBool::operator != (bool b)
	{ ET_ASSERT((_value & validMask) == 0); return b != (_value != 0); }

bool AtomicBool::operator != (const AtomicBool& r)
	{ ET_ASSERT((_value & validMask) == 0); return (r._value != 0) != (_value != 0); }

AtomicBool::operator bool() const
	{ ET_ASSERT((_value & validMask) == 0); return (_value != 0); }
