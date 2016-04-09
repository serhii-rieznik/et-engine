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
	template <typename T>
	struct ContainerBase
	{
		static size_t typeSize()
			{ return sizeof(T); }
	};
}
