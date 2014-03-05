/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
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

		virtual void startUpdates(TimerPool* timerPool = 0);
		virtual void update(float) {  }


	private:
		TimerPool* _owner;
		float _startTime;
		bool _running;
		bool _released;
	};
}
