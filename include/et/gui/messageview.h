/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/layout.h>
#include <et/gui/imageview.h>
#include <et/gui/label.h>
#include <et/gui/button.h>

namespace et
{
	namespace gui
	{
		enum MessageViewButton
		{
			MessageViewButton_First = 0x01,
			MessageViewButton_Second = 0x02,
			MessageViewButton_Any = 0xf0,
		};
		
		class MessageView : public ModalLayout
		{
		public:
			typedef IntrusivePtr<MessageView> Pointer;

		public:
			MessageView(const std::string& title, const std::string& text, Font font, const Image& bgImage = Image(),
						const std::string& button1title = "Close", const std::string& button2title = "Ok");

			void setContentOffset(const vec2& offset)
				{ _contentOffset = offset; }
			
			void setImage(const Image& img);
			void setBackgroundImage(const Image& img);
			void layout(const vec2&);
			
			void setText(const std::string& text);
			void setTitle(const std::string& text);
			
			void setButton1Title(const std::string&);
			void setButton2Title(const std::string&);
			void setButtonsBackground(const Image& img, State s);
			void setButtonsTextColor(const vec4& color);
			void setButtonsPressedTextColor(const vec4& color);
			
			void setButtonsEnabled(bool b1, bool b2, bool animated);

			ET_DECLARE_EVENT2(messageViewButtonSelected, MessageView*, MessageViewButton)
			
		private:
			void buttonClicked(Button* btn);
			
			bool hasFirstButton() const;
			bool hasSecondButton() const;
			
		private:
			ImageView::Pointer _imgBackground;
			ImageView::Pointer _imgImage;
			Label::Pointer _title;
			Label::Pointer _text;
			Button::Pointer _button1;
			Button::Pointer _button2;
			Button::Pointer _buttonCommon;
			
			vec2 _contentOffset;
		};
	}
}