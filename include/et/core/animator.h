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
#include <et/core/timerpool.h>

namespace et
{
class BaseAnimator : public TimedObject
{
public:
	BaseAnimator(TimerPool* tp) :
		TimedObject(tp)
	{
	}

	int64_t tag() const
	{
		return _tag;
	}

	void setTag(int64_t tag)
	{
		_tag = tag;
	}

protected:
	BaseAnimator(int64_t aTag, TimerPool* tp) :
		TimedObject(tp), _tag(aTag)
	{
	}

private:
	int64_t _tag = 0;
};

template <typename T>
inline T linearInterpolationFunction(const T& from, const T& to, float t)
{
	return from * (1.0f - t) + to * t;
}

inline float linearFunction(float t)
{
	return t;
}

template <typename T>
class Animator : public BaseAnimator
{
public:
	Animator() :
		BaseAnimator(0, nullptr), _from(), _to(), _value(), _valuePointer(nullptr),
		_startTime(0.0f), _duration(0.0f)
	{
		initDefaultInterpolators();
	}

	/*
	 * Just Timer Pool
	 */
	Animator(TimerPool* tp) :
		BaseAnimator(0, tp), _from(), _to(), _value(), _valuePointer(nullptr),
		_startTime(0.0f), _duration(0.0f)
	{
		initDefaultInterpolators();
	}

	Animator(TimerPool::Pointer tp) :
		BaseAnimator(0, tp.pointer()), _from(), _to(), _value(), _valuePointer(nullptr),
		_startTime(0.0f), _duration(0.0f)
	{
		initDefaultInterpolators();
	}

	/*
	 * Tag and Timer Pool
	 */
	Animator(int64_t tag, TimerPool* tp) :
		BaseAnimator(tag, tp), _from(), _to(), _value(), _valuePointer(nullptr),
		_startTime(0.0f), _duration(0.0f)
	{
		initDefaultInterpolators();
	}

	Animator(int64_t tag, TimerPool::Pointer tp) :
		BaseAnimator(tag, tp.pointer()), _from(), _to(), _value(), _valuePointer(nullptr),
		_startTime(0.0f), _duration(0.0f)
	{
		initDefaultInterpolators();
	}

	/*
	 * Value and Timer Pool
	 */
	Animator(const T& value, TimerPool* tp) :
		BaseAnimator(0, tp), _from(), _to(), _value(value), _valuePointer(nullptr),
		_startTime(0.0f), _duration(0.0f)
	{
		initDefaultInterpolators();
	}

	Animator(const T& value, TimerPool::Pointer tp) :
		BaseAnimator(0, tp.pointer()), _from(), _to(), _value(value), _valuePointer(nullptr),
		_startTime(0.0f), _duration(0.0f)
	{
		initDefaultInterpolators();
	}

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
		BaseAnimator(tag, tp.pointer()), _from(), _to(), _value(), _valuePointer(nullptr), _startTime(0.0f), _duration(0.0f)
	{
		initDefaultInterpolators();
		animate(value, from, to, duration);
	}

	const T& value() const
	{
		return _value;
	}

	void setValue(const T& v)
	{
		_value = v;
	}

	void animate(T* value, const T& from, const T& to, float duration)
	{
		if (duration == 0.0f)
		{
			_value = to;

			if (value != nullptr)
				*value = _value;

			updated.invoke();
			cancelUpdates();

			finished.invoke();
		}
		else
		{
			ET_ASSERT(timerPool() != nullptr);

			_valuePointer = value;
			_value = from;
			_from = from;
			_to = to;
			_duration = duration;

			startUpdates(timerPool());
			_startTime = actualTime();
		}
	};

	void animate(const T& from, const T& to, float duration)
	{
		animate(nullptr, from, to, duration);
	}

	void animate(const T& to, float duration)
	{
		animate(nullptr, _value, to, duration);
	}

	template <typename F>
	void setTimeInterpolationFunction(F func)
	{
		_timeInterpolationFunction = func;
	}

	template <typename F>
	void setValueInterpolationFunction(F func)
	{
		_valueInterpolationFunction = func;
	}

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

	T _from = T(0);
	T _to = T(0);
	T _value = T(0);

	T* _valuePointer = nullptr;

	float _startTime = 0.0f;
	float _duration = 0.0f;
};

typedef Animator<float> FloatAnimator;
typedef Animator<vec2> Vector2Animator;
typedef Animator<vec3> Vector3Animator;
typedef Animator<vec4> Vector4Animator;
typedef Animator<mat4> MatrixAnimator;
typedef Animator<rectf> RectAnimator;
}
