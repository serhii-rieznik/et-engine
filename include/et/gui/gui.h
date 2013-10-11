/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <deque>
#include <et/core/objectscache.h>
#include <et/rendering/rendercontext.h>
#include <et/input/input.h>
#include <et/gui/guirenderer.h>
#include <et/gui/layout.h>
#include <et/gui/fullscreenelement.h>
#include <et/gui/imageview.h>
#include <et/gui/label.h>
#include <et/gui/button.h>
#include <et/gui/listbox.h>
#include <et/gui/textfield.h>
#include <et/gui/scroll.h>
#include <et/gui/messageview.h>
#include <et/gui/slider.h>

namespace et
{
	namespace gui
	{
		class Gui : public Shared, public EventReceiver
		{
		public:
			typedef IntrusivePtr<Gui> Pointer;
			
		public:
			Gui(RenderContext* rc);
			
			void layout(const vec2& size);
			void render(RenderContext* rc);

			GuiRenderer& renderer() 
				{ return _renderer; }

			Layout::Pointer topmostLayout() const
				{ return _layouts.size() ? _layouts.back()->layout : Layout::Pointer(); }

			bool hasLayout(Layout::Pointer aLayout);
			
			void replaceTopmostLayout(Layout::Pointer newLayout,
				size_t animationFlags = AnimationFlag_None, float duration = 0.3f);
			
			void popTopmostLayout(size_t animationFlags = AnimationFlag_None,
				float duration = 0.3f);
			
			void replaceLayout(Layout::Pointer oldLayout, Layout::Pointer newLayout,
				size_t animationFlags = AnimationFlag_None, float duration = 0.3f);
			
			void removeLayout(Layout::Pointer oldLayout, size_t animationFlags = AnimationFlag_None,
				float duration = 0.3f);
			
			void pushLayout(Layout::Pointer newLayout, size_t animationFlags = AnimationFlag_None,
				float duration = 0.3f);

			void setBackgroundImage(const Image& img);

			bool pointerPressed(const et::PointerInputInfo&);
			bool pointerMoved(const et::PointerInputInfo&);
			bool pointerReleased(const et::PointerInputInfo&);
			bool pointerCancelled(const et::PointerInputInfo&);
			bool pointerScrolled(const et::PointerInputInfo&);
			
			bool characterEntered(size_t);
			
			void showMessageView(MessageView::Pointer mv,
				size_t animationFlags = AnimationFlag_None, float duration = 0.3f);
			
			ET_DECLARE_EVENT1(layoutDidAppear, Layout::Pointer)
			ET_DECLARE_EVENT1(layoutDidDisappear, Layout::Pointer)
			ET_DECLARE_EVENT1(layoutWillAppear, Layout::Pointer)
			ET_DECLARE_EVENT1(layoutWillDisappear, Layout::Pointer)

		private:
			class LayoutEntryObject;
			
			struct LayoutPair
			{
				Layout::Pointer oldLayout;
				Layout::Pointer newLayout;
				
				LayoutPair(Layout::Pointer o, Layout::Pointer n) :
					oldLayout(o), newLayout(n) { }
			};

			void buildLayoutVertices(RenderContext* rc, RenderingElement::Pointer element,
				Layout::Pointer layout);
			
			void buildBackgroundVertices(RenderContext* rc);

			void onKeyboardNeeded(Layout* l, Element* e);
			void onKeyboardResigned(Layout* l);

			void getAnimationParams(size_t flags, vec3* nextSrc, vec3* nextDst, vec3* currDst);

			void removeLayoutFromList(Layout::Pointer);
			void removeLayoutEntryFromList(LayoutEntryObject*);
			void layoutEntryTransitionFinished(LayoutEntryObject*);
			
			LayoutEntryObject* entryForLayout(Layout::Pointer);
			bool animatingTransition();
			
			void animateLayoutAppearing(Layout::Pointer, LayoutEntryObject* newEntry,
				size_t animationFlags, float duration);
			
			void onMessageViewButtonClicked(MessageView* view, MessageViewButton);

			void internal_replaceTopmostLayout(Layout::Pointer, AnimationDescriptor);
			void internal_popTopmostLayout(AnimationDescriptor desc);
			void internal_replaceLayout(LayoutPair, AnimationDescriptor);
			void internal_removeLayout(Layout::Pointer, AnimationDescriptor);
			void internal_pushLayout(Layout::Pointer, AnimationDescriptor);
			
		private:
			class LayoutEntryObject : public Shared, public AnimatorDelegate
			{
			public:
				enum State
				{
					State_Still,
					State_Appear,
					State_Disappear,
				};

			public:
				LayoutEntryObject(Gui* own, RenderContext* rc, Layout::Pointer l);
				void animateTo(const vec3& oa, float duration, State s);
				
			private:
				LayoutEntryObject(LayoutEntryObject&& l);
				LayoutEntryObject(LayoutEntryObject& l);
				LayoutEntryObject& operator = (LayoutEntryObject& l);
				
			private:
				void animatorUpdated(BaseAnimator*);
				void animatorFinished(BaseAnimator*);
				void moveDelegate();

			public:
				Gui* owner;
				Layout::Pointer layout;
				AutoPtr<Vector3Animator> animator;
				vec3 offsetAlpha;
				State state;
			};
			
			typedef IntrusivePtr<LayoutEntryObject> LayoutEntry;
			typedef std::deque<LayoutEntry> LayoutEntryStack;
			friend class LayoutEntryObject;

		private:
			RenderContext* _rc;
			GuiRenderer _renderer;

			RenderingElement::Pointer _renderingElementBackground;
			ImageView _background;
			
			Layout::Pointer _keyboardFocusedLayout;
			Element::Pointer _keyboardFocusedElement;

			LayoutEntryStack _layouts;
			vec2 _screenSize;
			bool _backgroundValid;
		};

	}
}
