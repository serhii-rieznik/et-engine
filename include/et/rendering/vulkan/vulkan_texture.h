/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/texture.h>

namespace et
{
	class VulkanTexture : public Texture
	{
	public:
		ET_DECLARE_POINTER(VulkanTexture);
		
	public:
		void bind(uint32_t) override;
        void update(TextureDescription::Pointer) override;
	};
}
