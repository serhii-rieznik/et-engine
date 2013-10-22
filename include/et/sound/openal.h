/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#if (ET_PLATFORM_MAC || ET_PLATFORM_IOS)
#	include <OpenAL/al.h>
#	include <OpenAL/alc.h>
#elif (ET_PLATFORM_ANDROID)
#	include <oal/al.h>
#	include <oal/alc.h>
#elif (ET_PLATFORM_WIN)
#	include <oal\al.h>
#	include <oal\alc.h>
#endif