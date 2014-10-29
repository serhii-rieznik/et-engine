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
	namespace gui
	{
		class TextureAtlas
		{
		public:
			TextureAtlas();
			TextureAtlas(RenderContext* rc, const std::string& filename, ObjectsCache& cache);
			
			bool loaded() const
				{ return _loaded; }
			
			void loadFromFile(RenderContext* rc, const std::string& filename, ObjectsCache& cache);
			void unload();
			
			const gui::Image& image(const std::string& key) const;
			gui::ImageList imagesForTexture(Texture t) const;
			
			Texture firstTexture() const;
			
		private:
			typedef std::map<std::string, Texture> TextureMap;

			TextureMap _textures;
			gui::ImageMap _images;
			bool _loaded;
		};
	}
}
