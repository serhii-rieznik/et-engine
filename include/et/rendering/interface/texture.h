/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>
#include <et/core/containers.h>

namespace et
{
class Texture : public LoadableObject
{
public:
	ET_DECLARE_POINTER(Texture);

	enum : uint32_t
	{
		minCompressedBlockWidth = 4,
		minCompressedBlockHeight = 4,
		minCompressedBlockDataSize = 32,
	};

	enum Flags : uint32_t
	{
		RenderTarget = 1 << 0,
		CopySource = 1 << 1,
		CopyDestination = 1 << 2,
	};

	struct Description
	{
		vec2i size = vec2i(0, 0);
		TextureFormat format = TextureFormat::Invalid;
		TextureTarget target = TextureTarget::Texture_2D;
		TextureDataLayout dataLayout = TextureDataLayout::FacesFirst;
		uint32_t levelCount = 1;
		uint32_t layersCount = 1;
		uint32_t flags = 0;

		vec2i sizeForMipLevel(uint32_t level) const;
		uint32_t dataOffsetForLayer(uint32_t layer, uint32_t level) const;
		uint32_t dataOffsetForMipLevel(uint32_t level, uint32_t layer) const;
		uint32_t dataSizeForAllMipLevels() const;
		uint32_t dataSizeForMipLevel(uint32_t level) const;
	};

public:
	Texture() = default;

	Texture(const Description& desc) :
		_desc(desc)
	{
	}

	const Description& description() const
	{
		return _desc;
	}

	TextureFormat format() const
	{
		return _desc.format;
	}

	TextureTarget target() const
	{
		return _desc.target;
	}

	vec2i size(uint32_t level) const
	{
		return _desc.sizeForMipLevel(level);
	}

	vec2 sizeFloat(uint32_t level) const
	{
		const vec2i sz = size(level);
		return vec2(static_cast<float>(sz.x), static_cast<float>(sz.y));
	}

	vec2 texel(uint32_t level) const
	{
		const vec2i sz = size(level);
		return vec2(1.0f / static_cast<float>(sz.x), 1.0f / static_cast<float>(sz.y));
	}

	vec2 getTexCoord(const vec2& vec, uint32_t level, TextureOrigin origin = TextureOrigin::TopLeft) const;

	virtual void setImageData(const BinaryDataStorage&) = 0;
	virtual void updateRegion(const vec2i& pos, const vec2i& size, const BinaryDataStorage&) = 0;

private:
	Description _desc;
};

inline vec2 Texture::getTexCoord(const vec2& vec, uint32_t level, TextureOrigin origin) const
{
	vec2 tx = texel(level);
	float ax = vec.x * tx.x;
	float ay = vec.y * tx.y;
	return vec2(ax, (origin == TextureOrigin::TopLeft) ? 1.0f - ay : ay);
}

inline vec2i Texture::Description::sizeForMipLevel(uint32_t level) const
{
	ET_ASSERT(level <= levelCount);
	return vec2i(size.x >> static_cast<int32_t>(level), size.y >> static_cast<int32_t>(level));
}

inline uint32_t Texture::Description::dataOffsetForMipLevel(uint32_t level, uint32_t layer) const
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

inline uint32_t Texture::Description::dataSizeForAllMipLevels() const
{
	uint32_t result = 0;
	for (uint32_t i = 0; i < levelCount; ++i)
		result += dataSizeForMipLevel(i);
	return result;
}

inline uint32_t Texture::Description::dataOffsetForLayer(uint32_t layer, uint32_t level) const
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

inline uint32_t Texture::Description::dataSizeForMipLevel(uint32_t level) const
{
	uint32_t bpp = bitsPerPixelForTextureFormat(format) / 8;

	uint32_t actualSize = static_cast<uint32_t>(sizeForMipLevel(level).square()) * bpp;
	uint32_t minimumSize = static_cast<uint32_t>(Texture::minCompressedBlockHeight * Texture::minCompressedBlockWidth) * bpp;
	
	return isCompressedTextureFormat(format) ? std::max(static_cast<uint32_t>(Texture::minCompressedBlockDataSize),
		std::max(minimumSize, actualSize)) : actualSize;
}

}
