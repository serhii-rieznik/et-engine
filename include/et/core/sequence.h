/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/core/timedobject.h>

namespace et
{
class Sequence : public TimedObject
{
public:
	enum class Curve : uint32_t
	{
		Linear,
		EasyIn,
		EasyOut
	};

	Sequence(float duration, float from = 0.0, float to = 1.0, Curve curve = Curve::Linear);

	void start(TimerPool::Pointer tp);
	void update(float t);

	ET_DECLARE_EVENT2(updated, Sequence*, float);
	ET_DECLARE_EVENT1(finished, Sequence*);

private:
	Curve _curve;
	float _startTime = 0.0f;
	float _endTime = 0.0f;
	float _duration = 0.0f;
	float _from = 0.0f;
	float _to = 0.0f;
	float _dt = 0.0f;
};
}
