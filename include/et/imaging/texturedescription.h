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
		vec2i sizeForMipLevel(size_t level)
		{
			vec2i result = size;
			for (size_t i = 0; i < level; ++i)
				result /= 2;
			return result;
		}

		size_t dataSizeForMipLevel(size_t level)
		{
			size_t actualSize = static_cast<size_t>(sizeForMipLevel(level).square()) * bitsPerPixel / 8;
			size_t minimumSize = static_cast<size_t>(minimalSizeForCompressedFormat.square()) * bitsPerPixel / 8;
			return compressed ? std::max(minimalDataSize, std::max(minimumSize, actualSize)) : actualSize;
		}

		size_t dataSizeForAllMipLevels()
		{
			size_t result = 0;
			for (size_t i = 0; i < mipMapCount; ++i)
				result += dataSizeForMipLevel(i);
			return result;
		}

		size_t dataOffsetForLayer(size_t layer, size_t level)
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

		size_t dataOffsetForMipLevel(size_t level, size_t layer)
		{
			size_t result = 0;
			if (dataLayout == TextureDataLayout::FacesFirst)
			{
				result = dataOffsetForLayer(layer, 0);
				for (size_t i = 0; i < level; ++i)
					result += dataSizeForMipLevel(i);
			}
			else if (dataLayout == TextureDataLayout::MipsFirst)
			{
				result = dataOffsetForLayer(layer, level);
				for (size_t l = 0; l < level; ++l)
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

	public:
		BinaryDataStorage data;

		vec2i size;
		vec2i minimalSizeForCompressedFormat;
		
		uint32_t compressed = 0;
		uint32_t bitsPerPixel = 0;
		uint32_t channels = 0;
		uint32_t mipMapCount = 0;
		uint32_t layersCount = 0;
		uint32_t alignment = 1;
		uint32_t rowSize = 0;

		size_t minimalDataSize = 0;
		
		TextureTarget target = TextureTarget::Texture_2D;
		TextureFormat internalformat = TextureFormat::Invalid;
		TextureFormat format = TextureFormat::Invalid;
		DataFormat type = DataFormat::UnsignedChar;
		
		TextureDataLayout dataLayout = TextureDataLayout::FacesFirst;
	};

}
