/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

#if (ET_PLATFORM_WIN)

#include <Windows.h>

using namespace et;

AtomicCounter::AtomicCounter() :
	_counter(0)
{
#if (ET_DEBUG)
	notifyOnRetain = false;
	notifyOnRelease = false;
#endif
}

AtomicCounterType AtomicCounter::retain()
{
#if (ET_DEBUG)
	if (notifyOnRetain)
		_CrtDbgBreak();
#endif

	return InterlockedIncrement(&_counter); 
}

AtomicCounterType AtomicCounter::release()
{
#if (ET_DEBUG)
	if (notifyOnRelease)
		_CrtDbgBreak();
#endif

	return InterlockedDecrement(&_counter);
}

void AtomicCounter::setValue(AtomicCounterType v)
{
	InterlockedExchange(&_counter, v);
}

static const AtomicCounterType validMask = static_cast<AtomicCounterType>(0xfffffffc);

AtomicBool::AtomicBool() : 
	_value(0) { }

bool AtomicBool::operator = (bool b)
{
	ET_ASSERT((_value & validMask) == 0);
	InterlockedExchange(&_value,  AtomicCounterType(b));
	return (_value != 0);
}

bool AtomicBool::operator == (bool b)
{
	ET_ASSERT((_value & validMask) == 0); 
	return b == (_value != 0);
}

bool AtomicBool::operator == (const AtomicBool& r)
{
	ET_ASSERT((_value & validMask) == 0);
	return (r._value != 0) == (_value != 0);
}

bool AtomicBool::operator != (bool b)
{
	ET_ASSERT((_value & validMask) == 0); 
	return b != (_value != 0);
}

bool AtomicBool::operator != (const AtomicBool& r)
{
	ET_ASSERT((_value & validMask) == 0);
	return (r._value != 0) != (_value != 0);
}

AtomicBool::operator bool() const
{
	ET_ASSERT((_value & validMask) == 0); 
	return (_value != 0);
}

#endif // ET_PLATFORM_WIN
