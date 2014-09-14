/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/guibase.h>

namespace et
{
	class TextureAtlasWriter
	{
	public:
		struct ImageItem
		{
			TextureDescription::Pointer image;
			gui::ImageDescriptor place;

			ImageItem(TextureDescription::Pointer t, const gui::ImageDescriptor& p) : 
				image(t), place(p) { }
		};

		typedef std::vector<ImageItem> ImageItemList;

		struct TextureAtlasItem
		{
			TextureDescription::Pointer texture;
			ImageItemList images;
			int maxWidth;
			int maxHeight;

		public:
			TextureAtlasItem() : 
				maxWidth(0), maxHeight(0) { }
		};

		typedef std::vector<TextureAtlasItem> TextureAtlasItemList;

	public:
		TextureAtlasWriter(bool addSpace = true) :
			_addSpace(addSpace) { }
		
		TextureAtlasItem& addItem(const vec2i& textureSize);
		bool placeImage(TextureDescription::Pointer image, TextureAtlasItem& item);

		const TextureAtlasItemList& items() const 
			{ return _items; }

		void writeToFile(const std::string& fileName, const char* textureNamePattern = "texture_%d.png");

	private:
		TextureAtlasItemList _items;
		bool _addSpace;
	};
}
