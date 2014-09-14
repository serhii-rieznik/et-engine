/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
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

const int defaultSpacing = 1;

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
	vec2i sz = image->size;
	vec2i offset;

	if (_addSpace)
	{
		if (sz.x + defaultSpacing < item.texture->size.x)
		{
			sz.x += defaultSpacing;
			offset.x = defaultSpacing;
		}

		if (sz.y + defaultSpacing < item.texture->size.y)
		{
			sz.y += defaultSpacing;
			offset.y = defaultSpacing;
		}
	}
	gui::ImageDescriptor desc(vec2(0.0f), vector2ToFloat(sz));

	if (item.images.size() == 0)
	{
		item.images.push_back(ImageItem(image, desc));

		if (desc.origin.x + desc.size.x > item.maxWidth) 
			item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - offset.x;

		if (desc.origin.y + desc.size.y > item.maxHeight) 
			item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - offset.y;

		return true;
	}
	
	for (ImageItemList::iterator i = item.images.begin(), e = item.images.end(); i != e; ++i)
	{
		desc.origin = i->place.origin + vec2(i->place.size.x, 0.0f);

		bool placed = (desc.origin.x + sz.x <= item.texture->size.x) && (desc.origin.y + sz.y <= item.texture->size.y);
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
				item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - offset.x;

			if (desc.origin.y + desc.size.y > item.maxHeight) 
				item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - offset.y;
			
			return true;
		}
	}

	for (ImageItemList::iterator i = item.images.begin(), e = item.images.end(); i != e; ++i)
	{
		desc.origin = i->place.origin + vec2(i->place.size.x, 0.0f);
		desc.origin = i->place.origin + vec2(0.0f, i->place.size.y);
		
		bool placed = (desc.origin.x + sz.x <= item.texture->size.x) && (desc.origin.y + sz.y <= item.texture->size.y);
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
				item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - offset.x;
			
			if (desc.origin.y + desc.size.y > item.maxHeight)
				item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - offset.y;
			
			return true;
		}
	}

	return false;
}

void TextureAtlasWriter::writeToFile(const std::string& fileName, const char* textureNamePattern)
{
	vec2 spacing = _addSpace ? vector2ToFloat(vec2i(defaultSpacing)) : vec2(0.0f);
	
	std::string path = addTrailingSlash(getFilePath(fileName));
	ArrayValue textures;
	ArrayValue images;
	
	int textureIndex = 0;
	for (TextureAtlasItemList::iterator i = _items.begin(), e = _items.end(); i != e; ++i, ++textureIndex)
	{
		BinaryDataStorage data(i->texture->size.square() * 4);
		data.fill(0);
		
		char textureName[1024] = { };
		sprintf(textureName, textureNamePattern, textureIndex);
		std::string texName = path + std::string(textureName);
		std::string texId = removeFileExt(textureName);
		
		Dictionary texture;
		texture.setStringForKey("filename", textureName);
		texture.setStringForKey("id", texId);
		textures->content.push_back(texture);
		
		int index = 0;
		for (const auto& ii : i->images)
		{
			TextureDescription image;
			png::loadFromFile(ii.image->origin(), image, true);

			std::string sIndex = intToStr(index);
			
			if (sIndex.length() < 2)
				sIndex = "0" + sIndex;
			
			std::string name = removeFileExt(getFileName(ii.image->origin()));

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
			
			Dictionary imageDictionary;
			imageDictionary.setStringForKey("name", name);
			imageDictionary.setStringForKey("texture", texId);
			imageDictionary.setArrayForKey("rect", rectToArray(rect(ii.place.origin, ii.place.size - spacing)));
			imageDictionary.setArrayForKey("offset", vec4ToArray(offset));
			images->content.push_back(imageDictionary);
			
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
				ImageOperations::transfer(image.data, image.size, components, data, i->texture->size, 4,
					vec2i(static_cast<int>(ii.place.origin.x), static_cast<int>(ii.place.origin.y)));
			}
			
			++index;
		}

		ImageWriter::writeImageToFile(texName, data, i->texture->size, 4, 8, ImageFormat_PNG, true);
	}
	
	Dictionary output;
	output.setArrayForKey("textures", textures);
	output.setArrayForKey("images", images);
	output.printContent();
	
	ET_FAIL("Serialization method not implemented")
}
