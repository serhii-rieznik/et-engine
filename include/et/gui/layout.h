/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/element2d.h>
#include <et/gui/imageview.h>

namespace et
{
	namespace gui
	{
		class Layout : public Element2d
		{
		public:
			typedef IntrusivePtr<Layout> Pointer;

		public:
			Layout();

			bool valid() const
				{ return _valid; }

			void layout(const vec2& sz);

			void addToRenderQueue(RenderContext* rc, GuiRenderer& gr);

			void update(float);
			void cancelDragging(float returnDuration = 0.0f);

			virtual void adjustVerticalOffset(float dy);
			virtual void resetVerticalOffset();
			
			void setActiveElement(Element* e);
			
			ET_DECLARE_EVENT2(layoutRequiresKeyboard, Layout*, Element*)
			ET_DECLARE_EVENT1(layoutDoesntNeedKeyboard, Layout*)
			
			vec2 contentSize();

		protected:
			friend class Gui;

			bool pointerPressed(const et::PointerInputInfo&);
			bool pointerMoved(const et::PointerInputInfo&);
			bool pointerReleased(const et::PointerInputInfo&);
			bool pointerScrolled(const et::PointerInputInfo&);

			void setInvalid();
			void collectTopmostElements(Element* element);

			Layout* owner()
				{ return this; }
			
			RenderingElement::Pointer renderingElement()
				{ return _renderingElement; }

			void initRenderingElement(et::RenderContext* rc);
			
			bool elementIsBeingDragged(gui::Element*);
			
		private:
			Element* activeElement(const PointerInputInfo& p);
			Element* getActiveElement(const PointerInputInfo& p, Element* e);
			void setCurrentElement(const PointerInputInfo& p, Element* e);
			void addElementToRenderQueue(Element* element, RenderContext* rc, GuiRenderer& gr);
			
			void performDragging(const PointerInputInfo&);

		private:
			RenderingElement::Pointer _renderingElement;
			
			Element* _currentElement;
			Element* _focusedElement;
			Element* _capturedElement;
			Element::List _topmostElements;
			vec2 _dragInitialPosition;
			vec2 _dragInitialOffset;
			bool _valid;
			bool _dragging;
		};

		class ModalLayout : public Layout
		{
		public:
			ModalLayout();
						
		protected:
			ImageView::Pointer backgroundFade()
				{ return _backgroundFade; }

			bool pointerPressed(const et::PointerInputInfo&);
			bool pointerMoved(const et::PointerInputInfo&);
			bool pointerReleased(const et::PointerInputInfo&);
			bool pointerScrolled(const et::PointerInputInfo&);
			
		private:
			ImageView::Pointer _backgroundFade;
		};
	}
}
