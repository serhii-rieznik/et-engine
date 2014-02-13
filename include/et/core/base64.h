/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/datastorage.h>

namespace et
{
	namespace base64
	{
		size_t decodedDataSize(const std::string&);
		BinaryDataStorage decode(const std::string&);
	}
}