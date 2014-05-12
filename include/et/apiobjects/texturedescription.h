/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containers.h>

namespace et
{
	enum TextureOrigin
	{
		TextureOrigin_TopLeft,
		TextureOrigin_BottomLeft
	};
	
	enum TextureDataLayout
	{
		TextureDataLayout_FacesFirst,
		TextureDataLayout_MipsFirst
	};

	class TextureDescription : public LoadableObject
	{  
	public:
		ET_DECLARE_POINTER(TextureDescription)
		typedef std::vector<TextureDescription::Pointer> List;

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
			return compressed ? etMax(minimalDataSize, etMax(minimumSize, actualSize)) : actualSize;
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
			if (dataLayout == TextureDataLayout_FacesFirst)
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
			if (dataLayout == TextureDataLayout_FacesFirst)
			{
				result = dataOffsetForLayer(layer, 0);
				for (size_t i = 0; i < level; ++i)
					result += dataSizeForMipLevel(i);
			}
			else if (dataLayout == TextureDataLayout_MipsFirst)
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
			{ return internalformat && format && (size.square() > 0); }

	public:
		BinaryDataStorage data;

		vec2i size;
		vec2i minimalSizeForCompressedFormat;
		
		uint32_t target = 0;
		int32_t internalformat = 0;
		uint32_t format = 0;
		uint32_t type = 0;
		uint32_t compressed = 0;
		
		size_t bitsPerPixel = 0;
		size_t channels = 0;
		size_t mipMapCount = 0;
		size_t layersCount = 0;
		size_t minimalDataSize = 0;
		
		TextureDataLayout dataLayout = TextureDataLayout_FacesFirst;
	};

}
