/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
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
		std::string encode(const BinaryDataStorage&);
	}
}
