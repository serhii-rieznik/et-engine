/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
#if defined(ET_ENABLE_AUTOVALUES)

	template <typename T>
	class AutoValue
	{
	public:
		AutoValue() : _value(0) { }

		T& operator = (const T& r) 
		{
			_value = r;
			return _value;
		}

		T& operator += (const T& r) 
		{
			_value += r;
			return _value;
		}

		T& operator -= (const T& r) 
		{
			_value -= r;
			return _value;
		}

		T& operator *= (const T& r) 
		{
			_value *= r;
			return _value;
		}

		T& operator /= (const T& r) 
		{
			_value /= r;
			return _value;
		}


		bool operator == (const T& r) const 
			{ return _value == r; }

		bool operator != (const T& r) const 
			{ return _value != r; }

		operator T() 
			{ return _value; }

		operator const T() const
			{ return _value; }

		T* operator &() 
			{ return &_value; }

		const T* operator &() const
			{ return &_value; }

	private:
		T _value;
	};

	typedef AutoValue<int> AutoInt;
	typedef AutoValue<float> AutoFloat;

#endif
}