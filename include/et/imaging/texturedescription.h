/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/rendering/rendering.h>

namespace et
{
	class TextureDescription : public LoadableObject
	{  
	public:
		ET_DECLARE_POINTER(TextureDescription);

	public:
		BinaryDataStorage data;

		vec2i size;
		TextureTarget target = TextureTarget::Texture_2D;
		TextureFormat format = TextureFormat::Invalid;
		TextureDataLayout dataLayout = TextureDataLayout::FacesFirst;
		uint32_t mipMapCount = 1;
		uint32_t layersCount = 1;
		bool isRenderTarget = false;

		const uint32_t minimalDataSize = 32;
		const vec2i minimalSizeForCompressedFormat = vec2i(4);

    public:
        TextureDescription();
        TextureDescription(const std::string& fileName);
        
        bool load(const std::string& name);
        bool preload(const std::string& name, bool fillWithZero);
        
		vec2i sizeForMipLevel(uint32_t level)
		{
			vec2i result = size;
			for (uint32_t i = 0; i < level; ++i)
				result /= 2;
			return result;
		}

		uint32_t dataSizeForMipLevel(uint32_t level)
		{
			uint32_t bpp = bitsPerPixelForTextureFormat(format) / 8;
			uint32_t actualSize = static_cast<uint32_t>(sizeForMipLevel(level).square()) * bpp;
			uint32_t minimumSize = static_cast<uint32_t>(minimalSizeForCompressedFormat.square()) * bpp;
			return isCompressedTextureFormat(format) ? std::max(minimalDataSize, std::max(minimumSize, actualSize)) : actualSize;
		}

		uint32_t dataSizeForAllMipLevels()
		{
			uint32_t result = 0;
			for (uint32_t i = 0; i < mipMapCount; ++i)
				result += dataSizeForMipLevel(i);
			return result;
		}

		uint32_t dataOffsetForLayer(uint32_t layer, uint32_t level)
		{
			if (dataLayout == TextureDataLayout::FacesFirst)
			{
				return dataSizeForAllMipLevels() * ((layer < layersCount) ?
					layer : (layersCount > 0 ? layersCount - 1 : 0));
			}
			else
			{
				return dataSizeForMipLevel(level) * ((layer < layersCount) ?
					layer : (layersCount > 0 ? layersCount - 1 : 0));

			}
		}

		uint32_t dataOffsetForMipLevel(uint32_t level, uint32_t layer)
		{
			uint32_t result = 0;
			if (dataLayout == TextureDataLayout::FacesFirst)
			{
				result = dataOffsetForLayer(layer, 0);
				for (uint32_t i = 0; i < level; ++i)
					result += dataSizeForMipLevel(i);
			}
			else if (dataLayout == TextureDataLayout::MipsFirst)
			{
				result = dataOffsetForLayer(layer, level);
				for (uint32_t l = 0; l < level; ++l)
					result += layersCount * dataSizeForMipLevel(l);
			}
			else
			{
				ET_FAIL("Invalid data layout");
			}
			
			return result;
		}
		
		bool valid() const
			{ return (size.square() > 0); }
	};

}
