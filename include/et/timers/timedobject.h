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

		virtual bool running() const
			{ return _running; }

		bool released() const
			{ return _released; }

		virtual void cancelUpdates();
		virtual void destroy();

	protected:
		friend class TimerPool;

		virtual void startUpdates(TimerPool* timerPool = 0);
		virtual void update(float) {  }

		float actualTime();

	private:
		TimerPool* _owner;
		bool _running;
		bool _released;
	};
}
