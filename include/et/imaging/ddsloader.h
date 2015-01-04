/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/rendering/texture.h>

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

