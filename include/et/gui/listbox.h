/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
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
		enum ListboxState
		{
			ListboxState_Default,
			ListboxState_Highlighted,
			ListboxState_Opened,
			ListboxState_max
		};

		enum ListboxPopupDirection
		{
			ListboxPopupDirection_Top,
			ListboxPopupDirection_Center,
			ListboxPopupDirection_Bottom,
		};

		class Listbox;

		class ListboxPopup : public Element2d
		{
		public:
			typedef IntrusivePtr<ListboxPopup> Pointer;

		public:
			ListboxPopup(Listbox* owner, const std::string& name = std::string());
			
			void setBackgroundImage(const Image& img);
			void addToRenderQueue(RenderContext*, GuiRenderer&);

			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			void pointerEntered(const PointerInputInfo&);
			void pointerLeaved(const PointerInputInfo&);

			void animatorUpdated(BaseAnimator*);
			void animatorFinished(BaseAnimator*);

			void hideText();
			void revealText();

		private:
			void buildVertices(GuiRenderer& gr);

		private:
			Listbox* _owner;
			GuiVertexList _backgroundVertices;
			GuiVertexList _selectionVertices;
			GuiVertexList _textVertices;
			FloatAnimator* _textAlphaAnimator;
			size_t _selectedIndex;
			float _textAlpha;
			bool _pressed;
		};

		class Listbox : public Element2d
		{
		public:
			typedef IntrusivePtr<Listbox> Pointer;

		public:
			Listbox(Font font, Element2d* parent, const std::string& name = std::string());

			void setImage(const Image& img, ListboxState state);
			void setBackgroundImage(const Image& img);
			void setSelectionImage(const Image& img);
			void setPopupDirection(ListboxPopupDirection d);

			void addToRenderQueue(RenderContext*, GuiRenderer&);

			void setFrame(const rect& r, float duration = 0.0f);
			bool containsPoint(const vec2& p, const vec2&);

			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			void pointerEntered(const PointerInputInfo&);
			void pointerLeaved(const PointerInputInfo&);

			void showPopup();
			void hidePopup();

			void resignFocus(Element*);

			void setValues(const StringList& v);
			void addValue(const std::string& v);

			size_t selectedIndex() const
				{ return _selectedIndex; }

			void setSelectedIndex(size_t value);

			void setPrefix(const std::string& prefix);

			const std::string& prefix() const
				{ return _prefix; }

			const StringList& values() const 
				{ return _values; }

			ET_DECLARE_EVENT1(popupOpened, Listbox*)
			ET_DECLARE_EVENT1(popupClosed, Listbox*)

		private:
			void buildVertices(GuiRenderer& gr);
			void configurePopup();

			void setState(ListboxState s);
			void onPopupAnimationFinished(Element2d*, AnimatedPropery);

			void popupDidOpen();

			bool shouldDrawText();
			
		private:
			friend class ListboxPopup;

			Font _font;
			ListboxPopup::Pointer _popup;
			Image _images[ListboxState_max];
			Image _background;
			Image _selection;
			GuiVertexList _backgroundVertices;
			GuiVertexList _textVertices;
			std::string _prefix;
			StringList _values;
			ListboxState _state;
			vec2 _contentOffset;
			size_t _selectedIndex;
			ListboxPopupDirection _direction;
			bool _popupOpened;
			bool _popupOpening;
			bool _popupValid;
			bool _mouseIn;
		};
	}
}