/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <Windows.h>
#include <et/threading/atomiccounter.h>

using namespace et;

AtomicCounter::AtomicCounter() : _counter(0)
	{ }

AtomicCounterType AtomicCounter::retain()
	{ return InterlockedIncrement(&_counter); }

AtomicCounterType AtomicCounter::release()
	{ return InterlockedDecrement(&_counter); }

static const AtomicCounterType validMask = static_cast<AtomicCounterType>(0xfffffffc);



AtomicBool::AtomicBool() : 
	_value(0) { }

bool AtomicBool::operator = (bool b)
{
	assert((_value & validMask) == 0);
	InterlockedExchange(&_value,  AtomicCounterType(b));
	return (_value != 0);
}

bool AtomicBool::operator == (bool b)
	{ assert((_value & validMask) == 0); return b == (_value != 0); }

bool AtomicBool::operator == (const AtomicBool& r)
	{ assert((_value & validMask) == 0); return (r._value != 0) == (_value != 0); }

bool AtomicBool::operator != (bool b)
	{ assert((_value & validMask) == 0); return b != (_value != 0); }

bool AtomicBool::operator != (const AtomicBool& r)
	{ assert((_value & validMask) == 0); return (r._value != 0) != (_value != 0); }

AtomicBool::operator bool() const
	{ assert((_value & validMask) == 0); return (_value != 0); }
