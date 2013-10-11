/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/apiobjects/texture.h>

namespace et
{
	class DDSLoader
	{
	public:
		static void loadFromStream(std::istream& stream, TextureDescription& desc);
		static void loadFromFile(const std::string& path, TextureDescription& desc);

		static void loadInfoFromStream(std::istream& stream, TextureDescription& desc);
		static void loadInfoFromFile(const std::string& path, TextureDescription& desc);
	};
}