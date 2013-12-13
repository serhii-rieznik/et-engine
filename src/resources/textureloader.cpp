/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/opengl/opengl.h>
#include <et/resources/textureloader.h>
#include <et/imaging/pngloader.h>
#include <et/imaging/ddsloader.h>
#include <et/imaging/pvrloader.h>

using namespace et;

TextureDescription::Pointer et::loadTextureDescription(const std::string& fileName, bool initWithZero)
{
	if (!fileExists(fileName))
		return TextureDescription::Pointer();
	
	std::string ext = getFileExt(fileName);
	TextureDescription* desc = nullptr;
	if (ext == "png")
	{
		desc = new TextureDescription;
		desc->target = GL_TEXTURE_2D;
		desc->setOrigin(fileName);
		png::loadInfoFromFile(fileName, *desc);
	}
	else if (ext == "dds")
	{
		desc = new TextureDescription;
		desc->target = GL_TEXTURE_2D;
		desc->setOrigin(fileName);
		dds::loadInfoFromFile(fileName, *desc);
	}
	else if (ext == "pvr")
	{
		desc = new TextureDescription;
		desc->target = GL_TEXTURE_2D;
		desc->setOrigin(fileName);
		pvr::loadInfoFromFile(fileName, *desc);
	}
	else if ((ext == "jpg") || (ext == "jpeg"))
	{
		assert("JPEG is not supported anymore" && false);
	}
	
	if ((desc != nullptr) && initWithZero)
	{
		desc->data = BinaryDataStorage(desc->dataSizeForAllMipLevels());
		desc->data.fill(0);
	}
	
	return TextureDescription::Pointer(desc);
}

TextureDescription::Pointer et::loadTexture(const std::string& fileName)
{
	if (!fileExists(fileName))
		return TextureDescription::Pointer();
	
	std::string ext = getFileExt(fileName);
	TextureDescription* desc = nullptr;
	
	if (ext == "png")
	{
		desc = new TextureDescription;
		desc->target = GL_TEXTURE_2D;
		desc->setOrigin(fileName);
		png::loadFromFile(fileName, *desc, true);
	}
	else if (ext == "dds")
	{
		desc = new TextureDescription;
		desc->target = GL_TEXTURE_2D;
		dds::loadFromFile(fileName, *desc);
	}
	else if (ext == "pvr")
	{
		desc = new TextureDescription;
		desc->target = GL_TEXTURE_2D;
		pvr::loadFromFile(fileName, *desc);
	}
	else if ((ext == "jpg") || (ext == "jpeg"))
	{
		assert("JPEG is not supported anymore" && false);
	}
	
	return TextureDescription::Pointer(desc);
}
