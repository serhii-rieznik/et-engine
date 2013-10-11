/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	template <typename T>
	struct ContainerBase
	{
		static const size_t typeSize()
			{ return sizeof(T); }
	};
}
