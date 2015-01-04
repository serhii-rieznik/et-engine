/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>
#include <et/core/hardware.h>

namespace et
{
	float queryContiniousTimeInSeconds();
	
	uint64_t queryContiniousTimeInMilliSeconds();
	
	uint64_t queryCurrentTimeInMicroSeconds();

	/**
	 * Returns device's screen size in native units.
	 * For Retina screens returns size in points
	 */
	vec2i nativeScreenSize();

	vec2i availableScreenSize();
	
	/**
	 * Reads application identifier from package
	 */
	std::string applicationIdentifierForCurrentProject();
	
	size_t streamSize(std::istream& s);
	
 	inline size_t roundToHighestPowerOfTwo(size_t x)
	{
		x = x - 1;
		x |= (x >> 1);
		x |= (x >> 2);
		x |= (x >> 4);
		x |= (x >> 8);
		x |= (x >> 16);
		return x + 1;
	}
	
	inline bool isPowerOfTwo(int x)
		{ return (x & (x - 1)) == 0; }
	
	inline bool isPowerOfTwo(size_t x)
		{ return (x & (x - 1)) == 0; }
}
