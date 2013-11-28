/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <algorithm>
#include <et/app/application.h>
#include <et/timers/timerpool.h>
#include <et/timers/timedobject.h>

using namespace et;

TimerPool::TimerPool(RunLoop* owner) : _owner(owner)
{
}

void TimerPool::attachTimedObject(TimedObject* obj)
{
	CriticalSectionScope lock(_lock);

	QueueEntry entry(obj, QueueAction_Update);
	if (std::find(_timedObjects.begin(), _timedObjects.end(), entry) != _timedObjects.end()) return;

	if (_updating)
	{
		entry.action = QueueAction_Add;
		if (std::find(_queue.begin(), _queue.end(), entry) == _queue.end())
			_queue.push_back(entry);
	}
	else
	{
		_timedObjects.push_back(entry);
	}
}

void TimerPool::detachTimedObject(TimedObject* obj)
{
	CriticalSectionScope lock(_lock);

	QueueEntry existsAddingEntry(obj, QueueAction_Add);
	auto existsAddingValue = std::find(_queue.begin(), _queue.end(), existsAddingEntry);
	if (existsAddingValue != _queue.end())
	{
		if (_updating)
			existsAddingEntry.action = QueueAction_Remove;
		else
			_queue.erase(existsAddingValue);
	}

	QueueEntry existsUpdatingEntry(obj, QueueAction_Update);
	auto existsUpdatingValue = std::find(_timedObjects.begin(), _timedObjects.end(), existsUpdatingEntry);
	if (existsUpdatingValue != _timedObjects.end()) 
	{
		if (_updating)
			existsUpdatingValue->action = QueueAction_Remove;
		else
			_timedObjects.erase(existsUpdatingValue);
	}
}

void TimerPool::update(float t)
{
	CriticalSectionScope lock(_lock);

	if (_queue.size() > 0)
	{
		for (auto& i : _queue)
		{
			if (i.action == QueueAction_Add)
			{
				i.action = QueueAction_Update;
				_timedObjects.push_back(i);
			}
		}
		_queue.clear();
	}

	_updating = true;

	for (auto i = _timedObjects.begin(); i != _timedObjects.end(); )
	{
		if ((i->action == QueueAction_Update) && i->object->running())
			i->object->update(t);
		
		if ((i->action == QueueAction_Remove) || !i->object->running())
			i = _timedObjects.erase(i);
		else 
			++i;
	}

	_updating = false;
}

void TimerPool::deleteTimedObjecct(TimedObject* obj)
{
	_owner->addTask(new DeletionTask<TimedObject>(obj), 0.0f);
}

float TimerPool::actualTime() const
{
	return _owner->time();
}

void TimerPool::retain()
{
	Shared::retain();
}

