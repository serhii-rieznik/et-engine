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

TimerPool::TimerPool(RunLoop* owner) :
	_owner(owner)
{
}

bool TimerPool::hasObjects()
{
	CriticalSectionScope lock(_lock);
	return !(_timedObjects.empty() && _queue.empty());
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

	for (auto& object : _timedObjects)
	{
		auto updatableObject = object.object;
		if ((object.action == QueueAction_Update) && updatableObject->running())
			updatableObject->update(t);
	}

	auto e = std::remove_if(_timedObjects.begin(), _timedObjects.end(), [](const QueueEntry& entry)
		{ return (entry.action == QueueAction_Remove) || (!entry.object->running()); });
	_timedObjects.erase(e, _timedObjects.end());

	_updating = false;
}

float TimerPool::actualTime() const
{
	return _owner->time();
}

void TimerPool::retain()
{
	Shared::retain();
}

