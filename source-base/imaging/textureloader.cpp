/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/imaging/textureloader.h>
#include <et/imaging/pngloader.h>
#include <et/imaging/ddsloader.h>
#include <et/imaging/pvrloader.h>
#include <et/imaging/hdrloader.h>
#include <et/imaging/jpegloader.h>
#include <et/imaging/tgaloader.h>
#include <et/imaging/bmploader.h>

using namespace et;

TextureDescription::Pointer et::loadTextureDescription(const std::string& fileName, bool initWithZero)
{
	if (!fileExists(fileName))
		return TextureDescription::Pointer();

	TextureDescription::Pointer desc = TextureDescription::Pointer::create();
	
	std::string ext = getFileExt(fileName);
	lowercase(ext);
	
	if (ext == "png")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		png::loadInfoFromFile(fileName, desc.reference());
	}
	else if (ext == "dds")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		dds::loadInfoFromFile(fileName, desc.reference());
	}
	else if (ext == "tga")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		tga::loadInfoFromFile(fileName, desc.reference());
	}
	else if (ext == "pvr")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		pvr::loadInfoFromFile(fileName, desc.reference());
	}
	else if (ext == "hdr")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		hdr::loadInfoFromFile(fileName, desc.reference());
	}
	else if ((ext == "jpg") || (ext == "jpeg"))
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		jpeg::loadInfoFromFile(fileName, desc.reference());
	}
	else if ((ext == "bmp"))
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		bmp::loadInfoFromFile(fileName, desc.reference());
	}
	else
	{
		ET_FAIL_FMT("Unsupported file extension: %s", ext.c_str());
	}
	
	if (desc.valid() && initWithZero)
	{
		desc->data = BinaryDataStorage(desc->dataSizeForAllMipLevels());
		desc->data.fill(0);
	}
	
	return desc;
}

TextureDescription::Pointer et::loadTexture(const std::string& fileName)
{
	if (!fileExists(fileName))
		return TextureDescription::Pointer();
	
	TextureDescription::Pointer desc = TextureDescription::Pointer::create();

	std::string ext = getFileExt(fileName);
	lowercase(ext);
	
	if (ext == "png")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		png::loadFromFile(fileName, desc.reference(), true);
	}
	else if (ext == "dds")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		dds::loadFromFile(fileName, desc.reference());
	}
	else if (ext == "tga")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		tga::loadFromFile(fileName, desc.reference());
	}
	else if (ext == "pvr")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		pvr::loadFromFile(fileName, desc.reference());
	}
	else if (ext == "hdr")
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		hdr::loadFromFile(fileName, desc.reference());
	}
	else if ((ext == "jpg") || (ext == "jpeg"))
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		jpeg::loadFromFile(fileName, desc.reference());
	}
	else if ((ext == "bmp"))
	{
		desc->target = TextureTarget::Texture_2D;
		desc->setOrigin(fileName);
		bmp::loadFromFile(fileName, desc.reference());
	}
	else
	{
		ET_FAIL_FMT("Unsupported file extension: %s", ext.c_str());
	}
	
	return desc;
}
