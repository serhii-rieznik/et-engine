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
    struct ApplicationIdentifier
    {
        std::string identifier;
        std::string companyName;
        std::string applicationName;
        
        ApplicationIdentifier() = default;
        
        ApplicationIdentifier(const std::string& aIdentifier, const std::string& aCompanyName,
            const std::string& aApplicationName) : identifier(aIdentifier), companyName(aCompanyName),
            applicationName(aApplicationName) { }
    };
    
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
        enum : size_t { DataSize = 256 };
        union
        {
            char data[DataSize];
            void* pointers[DataSize / sizeof(void*)];
        };
        
        PlatformDependentContext()
            { memset(data, DataSize, 0); }
    };
    
    class ApplicationContextFactory
    {
    public:
        virtual PlatformDependentContext createContextWithOptions(ContextOptions&) = 0;
        virtual void destroyContext(PlatformDependentContext) = 0;
        
        virtual ~ApplicationContextFactory() = default;
    };
}
