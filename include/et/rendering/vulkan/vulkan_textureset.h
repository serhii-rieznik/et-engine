/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/textureset.h>

namespace et
{
class VulkanState;
class VulkanRenderer;
class VulkanNativeTextureSet;
class VulkanTextureSetPrivate;
class VulkanTextureSet : public TextureSet
{
public:
	ET_DECLARE_POINTER(VulkanTextureSet);
		
public:
	VulkanTextureSet(VulkanRenderer*, VulkanState&, const Description&);
	~VulkanTextureSet();

	const VulkanNativeTextureSet& nativeSet() const;

private:
	ET_DECLARE_PIMPL(VulkanTextureSet, 128);
};
}
