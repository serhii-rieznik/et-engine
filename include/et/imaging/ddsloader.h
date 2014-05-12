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
	namespace dds
	{
		void loadFromStream(std::istream& stream, TextureDescription& desc);
		void loadFromFile(const std::string& path, TextureDescription& desc);

		void loadInfoFromStream(std::istream& stream, TextureDescription& desc);
		void loadInfoFromFile(const std::string& path, TextureDescription& desc);
	}
}

