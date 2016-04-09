/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/tools.h>

namespace et
{
	class IntervalTimer
	{
	public:
		IntervalTimer(bool runNow = false)
			{ if (runNow) run(); }

		void run() 
		{ 
			_runTimeMSec = queryContiniousTimeInMilliSeconds();
			_startTimeMSec = _runTimeMSec;
		}

		float lap()
		{
			_endTimeMSec = queryContiniousTimeInMilliSeconds();
			uint64_t dt = _endTimeMSec - _startTimeMSec;
			_startTimeMSec = _endTimeMSec;
			return static_cast<float>(dt) / 1000.0f;
		}

		float duration()
			{ return static_cast<float>(queryContiniousTimeInMilliSeconds() - _runTimeMSec) / 1000.0f; }
		
	private:
		uint64_t _runTimeMSec = 0;
		uint64_t _startTimeMSec = 0;
		uint64_t _endTimeMSec = 0;
	};
}
