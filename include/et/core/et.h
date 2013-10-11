/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <cmath>
#include <cstdlib>
#include <cstring>

#include <string>
#include <vector>
#include <map>

#include <iosfwd>
#include <iostream>

#include <limits>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#define ET_MAJOR_VERSION		0
#define ET_MINOR_VERSION		3

#define ET_CORE_INCLUDES

#include <et/platform/compileoptions.h>
#include <et/platform/platform.h>
#include <et/core/debug.h>
#include <et/core/constants.h>
#include <et/core/properties.h>
#include <et/core/log.h>
#include <et/core/strings.h>
#include <et/core/autoptr.h>
#include <et/core/intrusiveptr.h>
#include <et/core/dictionary.h>
#include <et/core/conversionbase.h>

#undef ET_CORE_INCLUDES