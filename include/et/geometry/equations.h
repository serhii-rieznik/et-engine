/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

template <typename T>
inline bool within(T value, T lowerBound, T upperBound)
	{ return (lowerBound <= value) && (upperBound >= value); }

template <typename T, typename F>
inline T resolveUsingDichotomy(T lowerBound, T upperBound, T precision, F func)
{
	ET_ASSERT(upperBound > lowerBound);
	
	if (within(func(upperBound), -precision, precision))
		return upperBound;
	
	T center = (lowerBound + upperBound) / static_cast<T>(2);
	while ((upperBound - lowerBound) > precision)
	{
		T xL = func(lowerBound);
		if (within(xL, -precision, precision))
			return lowerBound;
		
		T xM = func(center);
		if (within(xM, -precision, precision))
			return center;
		
		(xL * xM < 0) ? (upperBound = center) : (lowerBound = center);
		center = (lowerBound + upperBound) / static_cast<T>(2);
	}
	
	return center;
}