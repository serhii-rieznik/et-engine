/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/imaging/texturedescription.h>

namespace et
{
	class Texture : public LoadableObject
	{
	public:
		ET_DECLARE_POINTER(Texture);
		
	public:
        Texture()
            { }
        
        Texture(TextureDescription::Pointer desc) :
            _desc(desc) { }
        
        TextureDescription::Pointer description()
            { return _desc; }

        TextureDescription::Pointer description() const
            { return _desc; }
        
        void setDescription(TextureDescription::Pointer desc)
            { _desc = desc; }
        
        TextureFormat internalFormat() const
            { return _desc->internalformat; }
        
        TextureFormat format() const
            { return _desc->format; }
        
        DataFormat dataType() const
            { return _desc->type; }
        
        TextureTarget target() const
            { return _desc->target; }
        
        int width() const
            { return _desc->size.x; }
        
        int height() const
            { return _desc->size.y; }
        
        const vec2i& size() const
            { return _desc->size; }
        
        vec2 sizeFloat() const
            { return vec2(static_cast<float>(_desc->size.x), static_cast<float>(_desc->size.y)); }
        
        vec2 texel() const
            { return vec2(1.0f / static_cast<float>(_desc->size.x), 1.0f / static_cast<float>(_desc->size.y)); }

        virtual void bind(uint32_t) = 0;
        virtual void update(TextureDescription::Pointer) = 0;
        
    private:
        TextureDescription::Pointer _desc;
	};
}
