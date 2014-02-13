/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <fstream>
#include <et/core/et.h>
#include <et/core/conversion.h>
#include <et/core/cout.h>
#include <et/opengl/opengl.h>
#include <et/imaging/imagewriter.h>
#include <et/imaging/pngloader.h>
#include <et/imaging/imageoperations.h>
#include <et/gui/textureatlaswriter.h>

using namespace et;
using namespace et::gui;

TextureAtlasWriter::TextureAtlasItem& TextureAtlasWriter::addItem(const vec2i& textureSize)
{
	_items.push_back(TextureAtlasItem());
	
	TextureAtlasItem& item = _items.back();
	item.texture = et::TextureDescription::Pointer(new et::TextureDescription);
	item.texture->size = textureSize;
	
	return item;
}

bool TextureAtlasWriter::placeImage(TextureDescription::Pointer image, TextureAtlasItem& item)
{
	int w = image->size.x;
	int h = image->size.y;

	int xOffset = 0;
	int yOffset = 0;

	if (_addSpace)
	{
		if (w < item.texture->size.x - 1)
		{
			w++;
			xOffset = 1;
		}

		if (h < item.texture->size.y - 1)
		{
			h++;
			yOffset = 1;
		}
	}

	vec2 size(static_cast<float>(w), static_cast<float>(h));
	gui::ImageDescriptor desc(vec2(0.0f), size);

	if (item.images.size() == 0)
	{
		item.images.push_back(ImageItem(image, desc));

		if (desc.origin.x + desc.size.x > item.maxWidth) 
			item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - xOffset;

		if (desc.origin.y + desc.size.y > item.maxHeight) 
			item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - yOffset;

		return true;
	}
	
	for (ImageItemList::iterator i = item.images.begin(), e = item.images.end(); i != e; ++i)
	{
		desc.origin = i->place.origin + vec2(i->place.size.x, 0.0f);

		bool placed = (desc.origin.x + w <= item.texture->size.x) && (desc.origin.y + h <= item.texture->size.y);
		if (placed)
		{
			for (ImageItemList::iterator ii = item.images.begin(); ii != e; ++ii)
			{
				if ((ii != i) && ii->place.rectangle().intersects(desc.rectangle()))
				{
					placed = false;
					break;
				}
			}
		}

		if (placed)
		{
			item.images.push_back(ImageItem(image, desc));

			if (desc.origin.x + desc.size.x > item.maxWidth) 
				item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - xOffset;

			if (desc.origin.y + desc.size.y > item.maxHeight) 
				item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - yOffset;
			
			return true;
		}
	}

	for (ImageItemList::iterator i = item.images.begin(), e = item.images.end(); i != e; ++i)
	{
		desc.origin = i->place.origin + vec2(i->place.size.x, 0.0f);
		desc.origin = i->place.origin + vec2(0.0f, i->place.size.y);
		
		bool placed = (desc.origin.x + w <= item.texture->size.x) && (desc.origin.y + h <= item.texture->size.y);
		
		if (placed)
		{
			for (ImageItemList::iterator ii = item.images.begin(); ii != e; ++ii)
			{
				if ((ii != i) && ii->place.rectangle().intersects(desc.rectangle()))
				{
					placed = false;
					break;
				}
			}
		}

		if (placed)
		{
			item.images.push_back(ImageItem(image, desc));
			if (desc.origin.x + desc.size.x > item.maxWidth)
				item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - xOffset;
			if (desc.origin.y + desc.size.y > item.maxHeight)
				item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - yOffset;
			return true;
		}
	}

	return false;
}

void TextureAtlasWriter::writeToFile(const std::string& fileName, const char* textureNamePattern)
{
	std::string path = addTrailingSlash(getFilePath(fileName));
	std::ofstream descFile(fileName.c_str());

	int textureIndex = 0;
	for (TextureAtlasItemList::iterator i = _items.begin(), e = _items.end(); i != e; ++i, ++textureIndex)
	{
		BinaryDataStorage data(i->texture->size.square() * 4);
		data.fill(0);
		
		char textureName[1024] = { };
		sprintf(textureName, textureNamePattern, textureIndex);
		std::string texName = path + std::string(textureName);
		descFile << "texture: " << textureName << std::endl;

		int index = 0;
		for (ImageItemList::iterator ii = i->images.begin(), ie = i->images.end(); ii != ie; ++ii, ++index)
		{
			TextureDescription image;
			png::loadFromFile(ii->image->origin(), image, true);

			vec2i iOrigin(static_cast<int>(ii->place.origin.x), static_cast<int>(ii->place.origin.y));

			std::string sIndex = intToStr(index);
			
			if (sIndex.length() < 2)
				sIndex = "0" + sIndex;
			
			std::string name = removeFileExt(getFileName(ii->image->origin()));

			vec4 offset;
			size_t delimPos = name.find_first_of("~");
			if (delimPos != std::string::npos)
			{
				std::string params = name.substr(delimPos + 1);
				name.erase(delimPos);
				while (params.length())
				{
					size_t dPos = params.find_first_of("-");
					if (dPos == std::string::npos) break;

					std::string token = params.substr(0, dPos + 1);
					params.erase(0, dPos + 1);
					
					if (token == "offset-")
					{
						offset = strToVector4(params);
						for (size_t i = 0; i < 4; ++i)
						{
							if (offset[i] < 1.0f)
							{
								offset[i] *= (i % 2 == 0) ? static_cast<float>(image.size.x) :
									static_cast<float>(image.size.y);
							}
						}
					}
					else 
					{
						log::warning("Unrecognized token: %s", token.c_str());
						break;
					}
				}
			}

			descFile << "image: { name: \"" << name << "\" "
				"texture: \"" << textureName << "\"" << " "
				"rect: \"" << iOrigin << ";" << image.size << "\" "
				"offset: \""  << offset <<  "\" }" << std::endl;

			int components = 0;
			switch (image.format)
			{
			case GL_RGB:
				{
					components = 3;
					break;
				}
			case GL_RGBA:
				{
					components = 4;
					break;
				}
			default:
				break;
			};

			if (components)
			{
				ImageOperations::transfer(image.data, image.size, components,
					data, i->texture->size, 4, iOrigin);
			}
		}

		ImageWriter::writeImageToFile(texName, data, i->texture->size, 4, 8, ImageFormat_PNG, true);
	}
}


