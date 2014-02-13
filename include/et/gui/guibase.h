/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#pragma once

#include <map>

#include <et/core/et.h>
#include <et/core/flags.h>
#include <et/core/hierarchy.h>
#include <et/input/input.h>
#include <et/timers/animator.h>
#include <et/gui/guibaseconst.h>
#include <et/gui/guibaseclasses.h>

namespace et
{
	class RenderContext;
	
	namespace gui
	{
		class Element;
		class Layout;
		
		typedef Hierarchy<Element, Object> ElementHierarchy;
		class Element : public ElementHierarchy, public FlagsHolder, public EventReceiver, public TimedObject
		{
		public:
			typedef IntrusivePtr<Element> Pointer;

			union 
			{
				size_t tag;
				int tag_i;
			};

		public:
			Element(Element* parent, const std::string& name);
			virtual ~Element() { }

			void setParent(Element* element);

			void invalidateTransform();
			void invalidateContent();
			
			virtual bool enabled() const;
			virtual void setEnabled(bool enabled);

			virtual void broardcastMessage(const GuiMessage&);
			virtual void processMessage(const GuiMessage&) { }

			virtual void addToRenderQueue(RenderContext*, GuiRenderer&);
			virtual void addToOverlayRenderQueue(RenderContext*, GuiRenderer&);

			virtual bool pointerPressed(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual bool pointerMoved(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual bool pointerReleased(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual bool pointerScrolled(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual bool pointerCancelled(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual void pointerEntered(const PointerInputInfo&) { }
			
			virtual void pointerLeaved(const PointerInputInfo&) { }

			virtual bool capturesPointer() const
				{ return false; }
			
			virtual bool containLocalPoint(const vec2&)
				{ return false; }
			
			virtual vec2 positionInElement(const vec2& p)
				{ return p; }

			virtual bool visible() const
				{ return true; }

			virtual float finalAlpha() const 
				{ return 1.0f; }

			virtual const mat4& finalTransform()
				{ return identityMatrix; }

			virtual const mat4& finalInverseTransform() 
				{ return identityMatrix; }

			virtual void layout(const vec2&) 
				{ layoutChildren(); }

			void setAutolayot(const ElementLayout&);
			
			void setAutolayot(const vec2& pos, LayoutMode pMode, const vec2& sz,
				LayoutMode sMode, const vec2& pivot);
			
			void setAutolayotRelativeToParent(const vec2& pos, const vec2& sz, const vec2& pivot);
			
			void setAutolayoutSizeMode(LayoutMode);
			
			void setAutolayoutMask(size_t);
			
			void fillParent();
			void centerInParent();

			virtual void autoLayout(const vec2& contextSize, float duration = 0.0f);

			virtual bool focused() const
				{ return false; }
			
			virtual void setFocus() { };
			virtual void resignFocus(Element*) { };

			virtual void didAppear() { }
			virtual void didDisappear() { }
			virtual void willAppear() { }
			virtual void willDisappear() { }

			void bringToFront(Element* c);
			void sendToBack(Element* c);

			Element* baseChildWithName(const std::string&);

			template <typename T>
			T* childWithName(const std::string& name)
				{ return static_cast<T*>(baseChildWithName(name)); }

			/*
			 * Required Methods
			 */
			virtual ElementRepresentation representation() const = 0;
			virtual vec2 origin() const = 0;
			
			virtual const vec2& position() const = 0;
			virtual const vec2& size() const = 0;
			virtual const vec2& pivotPoint() const = 0;

			virtual void setPosition(const vec2&, float duration = 0.0f) = 0;
			virtual void setSize(const vec2&, float duration = 0.0f) = 0;
			virtual void setFrame(const vec2&, const vec2&, float duration = 0.0f) = 0;
			virtual void setPivotPoint(const vec2& p, bool preservePosition = true) = 0;

			virtual bool containsPoint(const vec2&, const vec2&) = 0;
			
			virtual vec2 contentSize() = 0;

			/*
			 * Events
			 */
			ET_DECLARE_EVENT2(dragStarted, Element*, const ElementDragInfo&)
			ET_DECLARE_EVENT2(dragged, Element*, const ElementDragInfo&)
			ET_DECLARE_EVENT2(dragFinished, Element*, const ElementDragInfo&)
			
			ET_DECLARE_EVENT1(hoverStarted, Element*)
			ET_DECLARE_EVENT1(hoverEnded, Element*)

		protected:
			void setContentValid()
				{ _contentValid = true; }

			bool contentValid() const
				{ return _contentValid; }

			bool transformValid() const
				{ return _transformValid; }

			bool inverseTransformValid()
				{ return _inverseTransformValid; }

			virtual void setInvalid()
				{ if (parent()) parent()->setInvalid(); }

			void setTransformValid(bool v) 
				{ _transformValid = v; }

			void setIverseTransformValid(bool v)
				{ _inverseTransformValid = v; }

			virtual mat4 parentFinalTransform()
				{ return parent() ? parent()->finalTransform() : identityMatrix; }

			virtual void animatorUpdated(BaseAnimator*) 
				{ /* virtual */ }

			virtual void animatorFinished(BaseAnimator*) 
				{ /* virtual */ }

			virtual Layout* owner()
				{ return parent() ? parent()->owner() : 0; }

			void startUpdates();
			TimerPool::Pointer timerPool();

			void layoutChildren();

			Element* childWithNameCallback(const std::string&, Element*);

			ET_DECLARE_PROPERTY_GET_REF_SET_REF(std::string, name, setName)

		private:
			friend class Hierarchy<Element, Object>;

			Element(const Element&) : 
				ElementHierarchy(0) { }

			Element& operator = (const Element&)
				{ return *this; }

			void startUpdates(TimerPool* timerPool);

		private:
			ElementLayout _autoLayout;

			bool _enabled;
			bool _transformValid;
			bool _inverseTransformValid;
			bool _contentValid;
		};

		inline bool elementIsSelected(State s)
			{ return (s >= State_Selected) && (s < State_max); }

		State adjustState(State s);
		float alignmentFactor(Alignment a);
	}
}
