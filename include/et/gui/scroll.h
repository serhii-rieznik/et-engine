/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/element2d.h>

namespace et
{
	namespace gui
	{
		class Scroll : public Element2d
		{
		public:
			typedef IntrusivePtr<Scroll> Pointer;
			
			enum Bounce
			{
				Bounce_Horizontal = 0x01,
				Bounce_Vertical = 0x02,
			};
			
		public:
			Scroll(Element2d* parent, const std::string& name = std::string());
			
			void setBounce(size_t);
			
			void setContentSize(const vec2& cs);
			
			void setOffset(const vec2& aOffset, float duration = 0.0f);
			void applyOffset(const vec2& dOffset, float duration = 0.0f);
			
			vec2 contentSize();
			void adjustContentSize();
			
			void setBackgroundColor(const vec4& color);
			void setScrollbarsColor(const vec4&);
						
			const vec2& offset() const
				{ return _offset; }
			
			void scrollToBottom(float duration = 0.0f);
						
		protected:
			virtual void setOffsetDirectly(const vec2& o);
			
		private:
			void buildVertices(RenderContext* rc, GuiRenderer& r);
			
			void addToRenderQueue(RenderContext*, GuiRenderer&);
			void addToOverlayRenderQueue(RenderContext*, GuiRenderer&);
			
			const mat4& finalTransform();
			const mat4& finalInverseTransform();
			
			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerCancelled(const PointerInputInfo&);
			bool containsPoint(const vec2& p, const vec2& np);
			
			void invalidateChildren();
			void broadcastPressed(const PointerInputInfo&);
			void broadcastMoved(const PointerInputInfo&);
			void broadcastReleased(const PointerInputInfo&);
			void broadcastCancelled(const PointerInputInfo&);
			
			void update(float t);
						
			float scrollOutOfContentXSize() const;
			float scrollOutOfContentYSize() const;

			float scrollUpperLimit() const;
			float scrollUpperDefaultValue() const;
			float scrollLowerLimit() const;
			float scrollLowerDefaultValue() const;

			float scrollLeftLimit() const;
			float scrollLeftDefaultValue() const;
			float scrollRightLimit() const;
			float scrollRightDefaultValue() const;

			void updateBouncing(float deltaTime);
			
			bool horizontalBounce() const
				{ return (_bounce & Bounce_Horizontal) == Bounce_Horizontal; }
			
			bool verticalBounce() const
				{ return (_bounce & Bounce_Vertical) == Bounce_Vertical; }

		private:
			enum BounceDirection
			{
				BounceDirection_None,
				BounceDirection_ToNear,
				BounceDirection_ToFar
			};
			
		private:
			Element::Pointer _selectedElement;
			
			GuiVertexList _backgroundVertices;
			GuiVertexList _scrollbarsVertices;

			Vector2Animator _offsetAnimator;
			PointerInputInfo _currentPointer;
			PointerInputInfo _previousPointer;
			mat4 _localFinalTransform;
			mat4 _localInverseTransform;
			vec4 _backgroundColor;
			vec4 _scrollbarsColor;
			vec2 _contentSize;
			vec2 _offset;
			vec2 _velocity;
			vector2<BounceDirection> _bouncing;
			size_t _bounce;
			float _updateTime;
			float _scrollbarsAlpha;
			float _scrollbarsAlphaTarget;

			bool _pointerCaptured;
			bool _manualScrolling;
		};

	}
}