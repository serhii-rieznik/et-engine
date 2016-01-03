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
	TextureDescription::Pointer loadTextureDescription(const std::string& name, bool initWithZero);
	TextureDescription::Pointer loadTexture(const std::string& name);
}
