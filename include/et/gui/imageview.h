/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/element2d.h>
#include <et/gui/guirenderer.h>

namespace et
{
	namespace gui
	{
		class ImageView : public Element2d
		{
		public:
			typedef IntrusivePtr<ImageView> Pointer;

		public:
			enum ContentMode
			{
				ContentMode_Stretch,
				ContentMode_Center,
				ContentMode_Fit,
				ContentMode_FitAnyway,
				ContentMode_Fill,
				ContentMode_Tile,
				ContentMode_Crop,
			};

		public:
			ImageView(Element2d* parent,
				const std::string& name = std::string());
			
			ImageView(const Texture& texture, Element2d* parent,
				const std::string& name = std::string());

			ImageView(const Texture& texture, const ImageDescriptor& i, Element2d* parent,
				const std::string& name = std::string());
			
			ImageView(const Image&, Element2d* parent,
				const std::string& name = std::string());

			void addToRenderQueue(RenderContext*, GuiRenderer&);

			const Texture& texture() const 
				{ return _texture; }

			Texture& texture() 
				{ return _texture; }

			const ImageDescriptor& imageDescriptor() const
				{ return _descriptor; }

			const vec4& backgroundColor() const
				{ return _backgroundColor; }
			
			void setImageDescriptor(const ImageDescriptor& d);
			void setContentMode(ImageView::ContentMode cm);
			void setTexture(const Texture& t, bool updateDescriptor);
			void setImage(const Image& img);
			void setBackgroundColor(const vec4& color);
			
			vec2 contentSize();

			ImageDescriptor calculateImageFrame();

			ET_DECLARE_PROPERTY_GET_REF(vec2, actualImageOrigin)
			ET_DECLARE_PROPERTY_GET_REF(vec2, actualImageSize)

		private:
			void buildVertices(RenderContext*, GuiRenderer&);

		private:
			Texture _texture;
			GuiVertexList _vertices;
			ImageDescriptor _descriptor;
			ImageView::ContentMode _contentMode;
			vec4 _backgroundColor;
		};

		typedef std::vector<ImageView::Pointer> ImageViewList;
	}
}