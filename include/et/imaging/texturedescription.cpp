/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/imaging/pngloader.h>
#include <et/imaging/ddsloader.h>
#include <et/imaging/pvrloader.h>
#include <et/imaging/hdrloader.h>
#include <et/imaging/jpegloader.h>
#include <et/imaging/tgaloader.h>
#include <et/imaging/bmploader.h>
#include <et/imaging/texturedescription.h>

namespace et
{
    
TextureDescription::TextureDescription()
{
}
    
TextureDescription::TextureDescription(const std::string& fileName)
{
    load(fileName);
}

bool TextureDescription::preload(const std::string& fileName, bool initWithZero)
{
    if (!fileExists(fileName))
        return false;
    
    setOrigin(fileName);
    
    std::string ext = getFileExt(fileName);
    lowercase(ext);
    
    if (ext == "png")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        png::loadInfoFromFile(fileName, (*this));
    }
    else if (ext == "dds")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        dds::loadInfoFromFile(fileName, (*this));
    }
    else if (ext == "tga")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        tga::loadInfoFromFile(fileName, (*this));
    }
    else if (ext == "pvr")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        pvr::loadInfoFromFile(fileName, (*this));
    }
    else if (ext == "hdr")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        hdr::loadInfoFromFile(fileName, (*this));
    }
    else if ((ext == "jpg") || (ext == "jpeg"))
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        jpeg::loadInfoFromFile(fileName, (*this));
    }
    else if ((ext == "bmp"))
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        bmp::loadInfoFromFile(fileName, (*this));
    }
    else
    {
        ET_FAIL_FMT("Unsupported file extension: %s", ext.c_str());
    }
    
    if (initWithZero)
    {
        data = BinaryDataStorage(dataSizeForAllMipLevels());
        data.fill(0);
    }
    
    return dataSizeForAllMipLevels() > 0;
}

bool TextureDescription::load(const std::string& fileName)
{
    if (!fileExists(fileName))
        return false;
    
    setOrigin(fileName);
    
    std::string ext = getFileExt(fileName);
    lowercase(ext);
    
    if (ext == "png")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        png::loadFromFile(fileName, (*this), true);
    }
    else if (ext == "dds")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        dds::loadFromFile(fileName, (*this));
    }
    else if (ext == "tga")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        tga::loadFromFile(fileName, (*this));
    }
    else if (ext == "pvr")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        pvr::loadFromFile(fileName, (*this));
    }
    else if (ext == "hdr")
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        hdr::loadFromFile(fileName, (*this));
    }
    else if ((ext == "jpg") || (ext == "jpeg"))
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        jpeg::loadFromFile(fileName, (*this));
    }
    else if ((ext == "bmp"))
    {
        target = TextureTarget::Texture_2D;
        setOrigin(fileName);
        bmp::loadFromFile(fileName, (*this));
    }
    else
    {
        ET_FAIL_FMT("Unsupported file extension: %s", ext.c_str());
    }
    
    return dataSizeForAllMipLevels() > 0;
}

}
