/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/element2d.h>
#include <et/gui/font.h>

namespace et
{
	namespace gui
	{
		class Button : public Element2d
		{
		public:
			typedef IntrusivePtr<Button> Pointer;
			typedef std::list<Pointer> List;

			enum Type
			{
				Type_PushButton,
				Type_CheckButton
			};

			enum ImageLayout
			{
				ImageLayout_Left,
				ImageLayout_Right
			};
			
			enum ContentMode
			{
				ContentMode_Fit,
				ContentMode_ScaleMaxToMin
			};

		public:
			Button(const std::string& title, Font font, Element2d* parent,
				const std::string& name = std::string());

			void addToRenderQueue(RenderContext* rc, GuiRenderer& guiRenderer);
			
			void setImage(const Image& img);
			void setBackgroundForState(const Texture& tex, const ImageDescriptor& desc, State s);
			void setBackgroundForState(const Image& img, State s);

			void adjustSize(float duration = 0.0f);
			void adjustSizeForText(const std::string&, float duration = 0.0f);
			vec2 sizeForText(const std::string&);
			
			void setContentMode(ContentMode);

			ET_DECLARE_EVENT1(clicked, Button*)
			ET_DECLARE_EVENT1(pressed, Button*)
			ET_DECLARE_EVENT1(released, Button*)
			
			const Image& backgroundForState(State state) const
				{ return _background[state]; }

			bool capturePointer() const;

			const std::string& title() const 
				{ return _title; }

			void setTitle(const std::string& t);

			const Image& image() const
				{ return _image; }
			
			void setImageLayout(ImageLayout l);

			const vec2& textSize();

			void setTextColor(const vec4& color);
			const vec4& textColor() const;
			
			void setTextPressedColor(const vec4& color);
			const vec4& textPressedColor() const;
			
			void setBackgroundColor(const vec4& color);
			const vec4& backgroundColor() const;

			bool pointerPressed(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerCancelled(const PointerInputInfo&);
			void pointerEntered(const PointerInputInfo&);
			void pointerLeaved(const PointerInputInfo&);

			Button::Type type() const
				{ return _type; }

			void setType(Button::Type t);

			bool selected() const
				{ return _selected; }
			
			void setSelected(bool s);

			void setContentOffset(const vec2& o);
			
			vec2 contentSize();

		protected:
			void performClick();

		private:
			void buildVertices(RenderContext* rc, GuiRenderer& gr);
			void setCurrentState(State s);

		private:
			Font _font;
			std::string _title;
			GuiVertexList _bgVertices;
			GuiVertexList _textVertices;
			GuiVertexList _imageVertices;
			StaticDataStorage<Image, State_max> _background;
			Image _image;
			vec4 _textColor;
			vec4 _textPressedColor;
			vec4 _backgroundColor;
			vec2 _textSize;
			vec2 _contentOffset;

			Type _type;
			State _state;
			ImageLayout _imageLayout;
			ContentMode _contentMode;
			bool _pressed;
			bool _hovered;
			bool _selected;
		};
	}
}
