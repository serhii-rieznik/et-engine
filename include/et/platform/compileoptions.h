/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
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
#
#elif defined(__MACH__)
#
#	include <TargetConditionals.h>
#
#endif
