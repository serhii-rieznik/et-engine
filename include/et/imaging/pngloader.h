/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/apiobjects/texturedescription.h>

namespace et
{
	namespace png
	{
		void loadInfoFromStream(std::istream& stream, TextureDescription& desc);
		void loadFromStream(std::istream& stream, TextureDescription& desc, bool flip);
		
		void loadInfoFromFile(const std::string& path, TextureDescription& desc);
		void loadFromFile(const std::string& path, TextureDescription& desc, bool flip);
	}
}

