/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/texturedescription.h>

namespace et
{

	class PNGLoader
	{
	public:
		static void loadInfoFromStream(std::istream& stream, TextureDescription& desc);
		static void loadInfoFromFile(const std::string& path, TextureDescription& desc);

		static void loadFromStream(std::istream& stream, TextureDescription& desc, bool flip);
		static void loadFromFile(const std::string& path, TextureDescription& desc, bool flip);
	};

}

