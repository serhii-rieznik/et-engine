/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/core/timerpool.h>
#include <et/core/timedobject.h>

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

TimerPool* TimedObject::timerPool()
{
	return _owner ? _owner : currentTimerPool().ptr();
}

void TimedObject::startUpdates(TimerPool* timerPool)
{
	if (_released || _running) return;

	if (_running && _owner)
		_owner->detachTimedObject(this);

	_running = true;
	
	_owner = (timerPool == nullptr) ? currentTimerPool().ptr() : timerPool;
	_owner->attachTimedObject(this);
	
	_startTime = _owner->actualTime();
}

void TimedObject::cancelUpdates()
{
	_running = false;
	
	if (_owner)
		_owner->detachTimedObject(this);
}

float TimedObject::actualTime()
{
	return timerPool()->actualTime();
}
