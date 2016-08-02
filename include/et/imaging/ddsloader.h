/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/imaging/texturedescription.h>

namespace et
{
	namespace dds
	{
		void loadFromStream(std::istream& stream, TextureDescription& desc);
		void loadFromFile(const std::string& path, TextureDescription& desc);

		void loadInfoFromStream(std::istream& stream, TextureDescription& desc);
		void loadInfoFromFile(const std::string& path, TextureDescription& desc);
	}
}

