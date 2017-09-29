/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/tasks.h>

namespace et
{
class TimerPool;
class TimedObject
{
public:
	TimedObject();
	TimedObject(TimerPool*);

	virtual ~TimedObject();

	virtual void cancelUpdates();

	virtual bool running() const
	{
		return _running;
	}

	bool released() const
	{
		return _released;
	}

	float startTime() const
	{
		return _startTime;
	}

	float actualTime();

protected:
	friend class TimerPool;

	virtual void update(float) {}

	virtual void startUpdates(TimerPool* timerPool = nullptr);
	virtual TimerPool* timerPool();

private:
	TimerPool* _owner = nullptr;
	float _startTime = 0.0f;
	bool _running = false;
	bool _released = false;
};
}
