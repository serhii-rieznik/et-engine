/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/timers/notifytimer.h>
#include <et/gui/element2d.h>
#include <et/gui/font.h>

namespace et
{
	namespace gui
	{
		class TextField : public Element2d
		{
		public:
			typedef IntrusivePtr<TextField> Pointer;

		public:
			TextField(const Image& background, const std::string& text, Font font, 
				Element* parent, const std::string& name = std::string());

			void addToRenderQueue(RenderContext*, GuiRenderer&);

			const std::string& text() const;
			void setText(const std::string& s);

			void processMessage(const GuiMessage& msg);
			void setSecured(bool);

			void setFocus();
			void resignFocus(Element*);
			
			void setBackgroundColor(const vec4& color);
			
			ET_DECLARE_EVENT1(editingStarted, TextField*)
			ET_DECLARE_EVENT1(textChanged, TextField*)
			ET_DECLARE_EVENT1(editingFinished, TextField*)
			
		private:
			void buildVertices(RenderContext*, GuiRenderer&);
			void onCreateBlinkTimerExpired(NotifyTimer* t);

		private:
			Font _font;
			Image _background;
			std::string _text;
			CharDescriptorList _charList;
			GuiVertexList _imageVertices;
			GuiVertexList _textVertices;
			GuiVertexList _backgroundVertices;
			NotifyTimer _caretBlinkTimer;
			vec4 _backgroundColor;
			bool _secured;
			bool _caretVisible;
		};
	}
}
