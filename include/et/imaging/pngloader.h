/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
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

