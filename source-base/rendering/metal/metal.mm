/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal.h>

namespace et
{
namespace metal
{

MTLTextureType textureTargetValue(TextureTarget value, uint32_t samples)
{
    static const std::map<TextureTarget, MTLTextureType> _map =
    {
        {TextureTarget::Texture_2D, MTLTextureType2D},
        {TextureTarget::Texture_Cube, MTLTextureTypeCube},
        {TextureTarget::Texture_2D_Array, MTLTextureType2DArray},
        {TextureTarget::Texture_Rectangle, MTLTextureType(-1)},
    };
    
    if (samples > 1)
    {
        ET_ASSERT(value == TextureTarget::Texture_2D);
        return MTLTextureType2DMultisample;
    }
    
    ET_ASSERT(_map.count(value) > 0);
    return _map.at(value);
}
    
MTLPixelFormat textureFormatValue(TextureFormat value)
{
    static const std::map<TextureFormat, MTLPixelFormat> _map =
    {
        {TextureFormat::RGBA8, MTLPixelFormatRGBA8Unorm},
        {TextureFormat::RGBA16F, MTLPixelFormatRGBA16Float},
        {TextureFormat::RGBA32F, MTLPixelFormatRGBA32Float},
        // TODO : add more
    };
    ET_ASSERT(_map.count(value) > 0);
    return _map.at(value);
}
    
}
}
