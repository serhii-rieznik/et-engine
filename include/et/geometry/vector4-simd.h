/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

#if (ET_PLATFORM_IOS)
#	include "vector4-simd.neon.h"
#elif (ET_PLATFORM_MAC || ET_PLATFORM_WIN)
#	include "vector4-simd.sse.h"
#else
#	error Unsupported platform selected
#endif
