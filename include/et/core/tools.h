/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
float queryContiniousTimeInSeconds();
uint64_t queryContiniousTimeInMilliSeconds();
uint64_t queryCurrentTimeInMicroSeconds();

/*
 * Reads application identifier from package
 */
std::string applicationIdentifierForCurrentProject();

uint32_t streamSize(std::istream& s);

template <class I>
inline I roundToHighestPowerOfTwo(I x)
{
	static_assert(std::is_unsigned<I>::value && std::is_integral<I>::value, 
		"roundToHighestPowerOfTwo works with unsigned integral types only");

	x = x - 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}

template <class T>
inline bool isPowerOfTwo(T x)
{
	static_assert(std::is_unsigned<T>::value && std::is_integral<T>::value,
		"roundToHighestPowerOfTwo works with unsigned integral types only");

	return (x & (x - 1)) == 0;
}

inline bool platformHasHardwareKeyboard()
{
	return true;
}

/*
 * Platform tools
 */
enum class SelectFileMode : uint32_t
{
	Open,
	Save
};

enum class AlertType : uint32_t
{
	Information,
	Warning,
	Error
};

std::string selectFile(const StringList& types, SelectFileMode mode, const std::string& defaultName);
void alert(const std::string& title, const std::string& message, const std::string& button, AlertType type);

}
