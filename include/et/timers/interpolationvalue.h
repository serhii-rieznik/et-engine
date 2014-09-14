/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/geometry/geometry.h>
#include <et/timers/timedobject.h>

namespace et
{
	template <typename T>
	class InterpolationValue : public TimedObject
	{
	public:
		InterpolationValue() :
			_value(0), _targetValue(0), _latestDelta(0), _rate(1.0f), _updateTime(0.0f) { }
		
		ET_DECLARE_PROPERTY_GET_REF_SET_REF(T, value, setValue)
		ET_DECLARE_PROPERTY_GET_REF_SET_REF(T, targetValue, setTargetValue)
		ET_DECLARE_PROPERTY_GET_REF(T, latestDelta)
		
		ET_DECLARE_PROPERTY_GET_REF_SET_REF(float, rate, setRate)
		
	public:
		ET_DECLARE_EVENT0(updated)
		ET_DECLARE_EVENT1(valueUpdated, const T&)
		
	public:
		void run()
		{
			TimedObject::startUpdates(nullptr);
			_updateTime = actualTime();
		}
		
		void finishInterpolation()
			{ step(1.0f); }
		
		void cancelInterpolation()
			{ _targetValue = _value; step(1.0f); }
		
		void addTargetValue(const T& value)
			{ _targetValue += value; }
		
		void resetLatestDelta()
			{ _latestDelta = T(0); }
		
	private:
		void update(float t)
		{
			step(etMin(1.0f, _rate * (t - _updateTime)));
			_updateTime = t;
		}
		
		void step(float dt)
		{
			_latestDelta = (_targetValue - _value) * dt;
			if (length(_latestDelta) > 0.0)
			{
				_value += _latestDelta;
				valueUpdated.invoke(_value);
				updated.invoke();
			}
		}
		
	private:
		float _updateTime;
	};
}