/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
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

		void start(TimerPool*, float period, int64_t repeatCount = DontRepear);
		void start(TimerPool::Pointer, float period, int64_t repeatCount = DontRepear);
		void update(float);

		ET_DECLARE_EVENT1(expired, NotifyTimer*)

	private:
		float _endTime;
		float _period;
		int64_t _repeatCount;
	};
}
