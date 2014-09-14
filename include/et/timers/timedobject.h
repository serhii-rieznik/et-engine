/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/tasks/tasks.h>

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
		virtual void destroy();

		virtual bool running() const
			{ return _running; }

		bool released() const
			{ return _released; }
		
		float startTime() const
			{ return _startTime; }
		
		float actualTime();

	protected:
		friend class TimerPool;

		virtual void startUpdates(TimerPool* timerPool = nullptr);
		virtual void update(float) {  }

		virtual TimerPool* timerPool()
			{ return _owner; }

	private:
		TimerPool* _owner = nullptr;
		float _startTime = 0;
		bool _running = false;
		bool _released = false;
	};
}
