/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
    struct ContextOptions
    {
        enum Style : uint32_t
        {
            Borderless = 0x00,
            Caption = 0x01,
            Sizable = 0x02,
            Hidden = 0x04
        };
        
        enum class SizeClass : uint32_t
        {
            Predefined,
            FillWorkarea,
            Fullscreen
        };
        
        vec2i size;
        vec2i minimumSize;
        uint32_t style = Style::Caption;
        SizeClass sizeClass = SizeClass::Predefined;
        bool keepAspectOnResize = false;
        bool supportsHighResolution = true;
    };
    
    struct ApplicationParameters
    {
        ContextOptions context;
        bool shouldSuspendOnDeactivate = PlatformOptions::IsMobile;
        bool shouldPreserveRenderContext = false;
    };
    
    struct PlatformDependentContext
    {
		enum
		{
			ObjectsCount = 32
		};
		std::array<void*, ObjectsCount> objects;

        PlatformDependentContext()
		{
			std::fill(objects.begin(), objects.end(), nullptr);
		}
    };
    
    class ApplicationContextFactory
    {
    public:
        virtual PlatformDependentContext createContextWithOptions(ContextOptions&) = 0;
        virtual void destroyContext(PlatformDependentContext) = 0;
        
        virtual ~ApplicationContextFactory() = default;
    };
}
