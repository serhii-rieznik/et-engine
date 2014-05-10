/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/timers/timerpool.h>
#include <et/tasks/taskpool.h>

namespace et
{
	class RunLoop : public Shared
	{
	public:
		ET_DECLARE_POINTER(RunLoop)
		
	public:
		RunLoop();
		virtual ~RunLoop() { }
		
		float time() const
			{ return _time; }
		
		uint64_t timeMSec() const
			{ return _actualTimeMSec; }

		TimerPool::Pointer mainTimerPool()
			{ return _timerPools.front(); }
		
		void update(uint64_t t);
		void pause();
		void resume();

		void attachTimerPool(TimerPool::Pointer pool);
		void detachTimerPool(TimerPool::Pointer pool);
		void detachAllTimerPools();

		virtual void addTask(Task*, float);
		
		bool hasTasks()
			{ return _taskPool.hasTasks(); }

	protected:
		void updateTime(uint64_t t);

	private:
		std::vector<TimerPool::Pointer> _timerPools;
		TaskPool _taskPool;
		
		uint64_t _actualTimeMSec;
		uint64_t _activityTimeMSec;
		
		float _time;
		bool _started;
		bool _active;
	};
}
