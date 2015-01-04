/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#ifndef ET_CORE_INCLUDES
#	error This file should not be included from anywhere except et.h
#endif

#if defined(_MSC_VER)
#
#	pragma warning(disable:4201)
#	pragma warning(disable:4996)
#	pragma warning(disable:4131)
#	pragma warning(disable:4204)
#	pragma warning(disable:4099)
#
#elif defined(__MACH__)
#
#	include <TargetConditionals.h>
#
#endif
