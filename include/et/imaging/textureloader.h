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
	TextureDescription::Pointer loadTextureDescription(const std::string& name, bool initWithZero);
	TextureDescription::Pointer loadTexture(const std::string& name);
}
