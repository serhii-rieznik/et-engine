/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/runloop.h>
#include <et/core/tasks.h>

using namespace et;

RunLoop::RunLoop() 
{
	attachTimerPool(TimerPool::Pointer::create(this));
}

RunLoop::~RunLoop()
{
}

void RunLoop::update(uint64_t t)
{
	updateTime(t);

	if (_active) 
	{
		_taskPool.update(_time);
		for (auto& tp : _timerPools)
			tp->update(_time);
	}
}

void RunLoop::addTask(Task* t, float delay)
{
	_taskPool.addTask(t, delay);
}

void RunLoop::attachTimerPool(const TimerPool::Pointer& pool)
{
	if (std::find(_timerPools.begin(), _timerPools.end(), pool) == _timerPools.end())
	{
		_timerPools.push_back(pool);
		_timerPools.back()->setOwner(this);
	}
}

void RunLoop::detachTimerPool(const TimerPool::Pointer& pool)
{
	auto i = _timerPools.begin();
	while (i != _timerPools.end())
	{
		auto& p = *i;
		if (p == pool)
		{
			p->setOwner(nullptr);
			_timerPools.erase(i);
			break;
		}
		else 
		{
			++i;
		}
	}
}

void RunLoop::detachAllTimerPools()
{
	for (auto& i : _timerPools)
		i->setOwner(nullptr);
	
	_timerPools.clear();
}

void RunLoop::pause()
{
	_active = false;
}

void RunLoop::resume()
{
	if (_active) return;

	_active = true;
	_activityTimeMSec = _actualTimeMSec;
}

void RunLoop::updateTime(uint64_t t)
{
	_actualTimeMSec = t;
	
	if (!_started)
	{
		_started = true;
		_activityTimeMSec = _actualTimeMSec;
	}

	if (_active) 
	{
		_lastFrameTime = static_cast<float>(t - _activityTimeMSec) / 1000.0f;
		_time += _lastFrameTime;
		_activityTimeMSec = t;
	}
}
