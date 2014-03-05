/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/timers/timedobject.h>

namespace et
{
	class NotifyTimer : public TimedObject
	{
	public:
		enum
		{
			RepeatForever = -1,
			DontRepear = 0
		};

	public:
		NotifyTimer();

		void start(TimerPool::Pointer, float period, int64_t repeatCount = DontRepear);
		void update(float);

		ET_DECLARE_EVENT1(expired, NotifyTimer*)

	private:
		float _endTime;
		float _period;
		int64_t _repeatCount;
	};
}