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
    : Texture(desc)
{
    ET_PIMPL_INIT(MetalTexture);
    
    _private->textureDesc = [[MTLTextureDescriptor alloc] init];
    _private->textureDesc.textureType = metal::textureTargetValue(desc->target, 1); // TODO : add samples
    _private->textureDesc.pixelFormat = metal::textureFormatValue(desc->internalformat);
    _private->textureDesc.width = desc->size.x;
    _private->textureDesc.height = desc->size.y;
    _private->textureDesc.mipmapLevelCount = std::max(1u, desc->mipMapCount);
    _private->texture.texture = [metal.device newTextureWithDescriptor:_private->textureDesc];
    
    update(desc);
}
    
MetalTexture::~MetalTexture()
{
    ET_OBJC_RELEASE(_private->textureDesc);
    ET_PIMPL_FINALIZE(MetalTexture)
}
    
const MetalNativeTexture& MetalTexture::nativeTexture() const
{
    return _private->texture;
}

void MetalTexture::bind(uint32_t)
{
    
}

void MetalTexture::update(TextureDescription::Pointer desc)
{
    uint32_t aDataSize = desc->data.size();
    char* aDataPtr = desc->data.binary();
    
    if ((aDataSize == 0) || (aDataPtr == nullptr))
        return;
    
    if (desc->target == TextureTarget::Texture_2D)
    {
        for (size_t level = 0; level < desc->mipMapCount; ++level)
        {
            vec2i mipSize = description()->sizeForMipLevel(level);
            size_t mipOffset = description()->dataOffsetForMipLevel(level, 0);
            size_t mipDataSize = description()->dataSizeForMipLevel(level);
            MTLRegion region = MTLRegionMake2D(0, 0, mipSize.x, mipSize.y);
            const char* ptr = (aDataPtr && (mipOffset < aDataSize)) ? (aDataPtr + mipOffset) : nullptr;
            
            // TODO : implement proper bytesPerRow
            [_private->texture.texture replaceRegion:region mipmapLevel:level slice:0
                withBytes:ptr bytesPerRow:mipDataSize / mipSize.y bytesPerImage:mipDataSize];
        }
    }
    else if (desc->target == TextureTarget::Texture_Cube)
    {
        // TODO : implement
    }
}

}
