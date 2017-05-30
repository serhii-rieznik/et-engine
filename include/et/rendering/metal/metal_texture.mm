/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal_texture.h>
#include <et/rendering/metal/metal.h>

namespace et
{

class MetalTexturePrivate
{
public:
    MetalNativeTexture texture;
    MTLTextureDescriptor* textureDesc = nullptr;
};
    
MetalTexture::MetalTexture(MetalState& metal, TextureDescription::Pointer desc)
    : Texture(desc->desc())
{
    ET_PIMPL_INIT(MetalTexture);

    _private->textureDesc = [[MTLTextureDescriptor alloc] init];
    _private->textureDesc.textureType = metal::textureTargetValue(desc->target, 1); // TODO : add samples
    _private->textureDesc.pixelFormat = metal::textureFormatValue(desc->format);
    _private->textureDesc.width = desc->size.x;
    _private->textureDesc.height = desc->size.y;
    _private->textureDesc.mipmapLevelCount = std::max(1u, desc->levelCount);
    _private->texture.texture = [metal.device newTextureWithDescriptor:_private->textureDesc];

	if (desc->data.size() > 0)
		setImageData(desc->data);
}
    
MetalTexture::~MetalTexture()
{
    ET_OBJC_RELEASE(_private->textureDesc);
    ET_PIMPL_FINALIZE(MetalTexture);
}
    
const MetalNativeTexture& MetalTexture::nativeTexture() const
{
    return _private->texture;
}

void MetalTexture::setImageData(const BinaryDataStorage& data)
{
    uint32_t aDataSize = data.size();
	ET_ASSERT(aDataSize > 0);

    const char* aDataPtr = data.binary();
	ET_ASSERT(aDataPtr != nullptr);

    if (description().target == TextureTarget::Texture_2D)
    {
        for (uint32_t level = 0; level < description().levelCount; ++level)
        {
            vec2i mipSize = description().sizeForMipLevel(level);
            size_t mipOffset = description().dataOffsetForMipLevel(level, 0);
            size_t mipDataSize = description().dataSizeForMipLevel(level);
            MTLRegion region = MTLRegionMake2D(0, 0, mipSize.x, mipSize.y);
            const char* ptr = (aDataPtr && (mipOffset < aDataSize)) ? (aDataPtr + mipOffset) : nullptr;
            
            // TODO : implement proper bytesPerRow
            [_private->texture.texture replaceRegion:region mipmapLevel:level slice:0
                withBytes:ptr bytesPerRow:mipDataSize / mipSize.y bytesPerImage:mipDataSize];
        }
    }
    else if (description().target == TextureTarget::Texture_Cube)
    {
        // TODO : implement
    }
}

void MetalTexture::updateRegion(const vec2i& /* pos */, const vec2i& /* size */, const BinaryDataStorage&)
{
	ET_FAIL("Not implemented");
}

uint8_t* MetalTexture::map(uint32_t /* level */, uint32_t /* face */, uint32_t /* options */)
{
	return nullptr;
}

void MetalTexture::unmap()
{
}


}
