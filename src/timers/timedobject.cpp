/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/timers/timerpool.h>
#include <et/timers/timedobject.h>

using namespace et;

TimedObject::TimedObject() : 
	_owner(nullptr), _startTime(0.0f), _running(false), _released(false)
{
}

TimedObject::TimedObject(TimerPool* tp) :
	_owner(tp), _startTime(0.0f), _running(false), _released(false)
{

}

TimedObject::~TimedObject()
{
	if (_owner)
		_owner->detachTimedObject(this);
}

void TimedObject::startUpdates(TimerPool* timerPool)
{
	if (_released || _running) return;

	if (_running && _owner)
		_owner->detachTimedObject(this);

	_running = true;
	
	_owner = (timerPool == nullptr) ? mainRunLoop().mainTimerPool().ptr() : timerPool;
	_owner->attachTimedObject(this);
	
	_startTime = _owner->actualTime();
}

void TimedObject::cancelUpdates()
{
	_running = false;
	
	if (_owner)
		_owner->detachTimedObject(this);
}

void TimedObject::destroy()
{
	if (_released) return;

	_running = false;
	_released = true;
	
	(_owner ? _owner : mainTimerPool().ptr())->deleteTimedObjecct(this);
}

float TimedObject::actualTime()
{
	ET_ASSERT((_owner != nullptr) && "TimedObject isn't attached to timer pool");
	return _owner->actualTime();
}
