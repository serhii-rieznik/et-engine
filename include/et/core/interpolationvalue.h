/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/geometry/geometry.h>
#include <et/core/timedobject.h>

namespace et
{
	template <typename T>
	class InterpolationValue : public TimedObject
	{
	public:
		ET_DECLARE_EVENT0(updated)
		ET_DECLARE_EVENT1(valueUpdated, const T&)
		ET_DECLARE_EVENT0(finished)

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

		const T& value() const
			{ return _value; }

		void setValue(const T& value)
			{ _value = value; }

		const T& targetValue() const
			{ return _targetValue; }

		void setTargetValue(const T& targetValue)
			{ _targetValue = targetValue; }

		void setRate(float rate)
			{ _rate = rate; }

	private:
		void update(float t)
		{
			step(std::min(1.0f, _rate * (t - _updateTime)));
			_updateTime = t;
		}
		
		void step(float dt)
		{
			auto delta = _targetValue - _value;
			_latestDelta = delta * dt;
			if (length(delta) > std::numeric_limits<float>::epsilon())
			{
				_shouldInvokeFinish = true;
				_value += _latestDelta;
				valueUpdated.invoke(_value);
				updated.invoke();
			}
			else if (_shouldInvokeFinish)
			{
				finished.invoke();
				_shouldInvokeFinish = false;
			}
		}
		
	private:
		T _value = T(0);
		T _targetValue = T(0);
		T _latestDelta = T(0);
		float _updateTime = 0.0f;
		float _rate = 1.0f;
		bool _shouldInvokeFinish = false;
	};
}
