/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/timers/timedobject.h>
#include <et/timers/timerpool.h>

namespace et
{
	class BaseAnimator : public TimedObject
	{
	public:
		BaseAnimator(TimerPool* tp) :
			TimedObject(tp), _tag(0) { }

	public:
		ET_DECLARE_PROPERTY_GET_COPY_SET_COPY(int, tag, setTag)

	protected:
		BaseAnimator(int aTag, TimerPool* tp) :
			TimedObject(tp), _tag(aTag) { }
	};

	typedef std::vector<BaseAnimator*> AnimatorList;
	
	template <typename T>
	inline T linearInterpolationFunction(const T& from, const T& to, float t)
		{ return from * (1.0f - t) + to * t; }

	inline float linearFunction(float t)
		{ return t; }
	
	template <typename T>
	class Animator : public BaseAnimator
	{
	public:
		Animator() :
			BaseAnimator(0, nullptr), _from(), _to(), _value(), _valuePointer(nullptr),
			_startTime(0.0f), _duration(0.0f) { initDefaultInterpolators(); }
		
		/*
		 * Just Timer Pool
		 */
		Animator(TimerPool* tp) :
			BaseAnimator(0, tp), _from(), _to(), _value(), _valuePointer(nullptr),
			_startTime(0.0f), _duration(0.0f) { initDefaultInterpolators(); }

		Animator(TimerPool::Pointer tp) :
			BaseAnimator(0, tp.ptr()), _from(), _to(), _value(), _valuePointer(nullptr),
			_startTime(0.0f), _duration(0.0f) { initDefaultInterpolators(); }

		/*
		 * Tag and Timer Pool
		 */
		Animator(int tag, TimerPool* tp) :
			BaseAnimator(tag, tp), _from(), _to(), _value(), _valuePointer(nullptr),
			_startTime(0.0f), _duration(0.0f) { initDefaultInterpolators(); }

		Animator(int tag, TimerPool::Pointer tp) :
			BaseAnimator(tag, tp.ptr()), _from(), _to(), _value(), _valuePointer(nullptr),
			_startTime(0.0f), _duration(0.0f) { initDefaultInterpolators(); }

		/*
		 * All properties
		 */
		Animator(T* value, const T& from, const T& to, float duration, int tag, TimerPool* tp) :
			BaseAnimator(tag, tp), _from(), _to(), _value(), _valuePointer(nullptr), _startTime(0.0f), _duration(0.0f)
		{
			initDefaultInterpolators();
			animate(value, from, to, duration);
		}

		Animator(T* value, const T& from, const T& to, float duration, int tag, TimerPool::Pointer tp) :
			BaseAnimator(tag, tp.ptr()), _from(), _to(), _value(), _valuePointer(nullptr), _startTime(0.0f), _duration(0.0f)
		{
			initDefaultInterpolators();
			animate(value, from, to, duration);
		}

		const T& value() const
			{ return _value; }

		void animate(T* value, const T& from, const T& to, float duration)
		{
			ET_ASSERT(timerPool() != nullptr);

			_valuePointer = value;
			_value = from;
			_from = from;
			_to = to;
			_duration = duration;

			startUpdates(timerPool());
			_startTime = actualTime();
		};
		
		void animate(const T& from, const T& to, float duration)
			{ animate(nullptr, from, to, duration); }
		
		template <typename F>
		void setTimeInterpolationFunction(F func)
			{ _timeInterpolationFunction = func; }

		template <typename F>
		void setValueInterpolationFunction(F func)
			{ _valueInterpolationFunction = func; }
		
		ET_DECLARE_EVENT0(updated);
		ET_DECLARE_EVENT0(finished);
		
	private:
		void initDefaultInterpolators()
		{
			_timeInterpolationFunction = linearFunction;
			_valueInterpolationFunction = linearInterpolationFunction<T>;
		}
		
		void update(float t)
		{
			float dt = (t - _startTime) / _duration;
			if (dt >= 1.0f)
			{
				_value = _to;
				
				if (_valuePointer)
					*_valuePointer = _value;
								
				updated.invoke();
				cancelUpdates();
				finished.invoke();
			}
			else 
			{
				_value = _valueInterpolationFunction(_from, _to, _timeInterpolationFunction(dt));
				
				if (_valuePointer)
					*_valuePointer = _value;
				
				updated.invoke();
			}
		}

	private:
		std::function<float(float)> _timeInterpolationFunction;
		std::function<T(const T&, const T&, float)> _valueInterpolationFunction;
		
		T _from;
		T _to;
		T _value;
		
		T* _valuePointer;
		
		float _startTime;
		float _duration;
	};

	typedef Animator<float> FloatAnimator;

}
