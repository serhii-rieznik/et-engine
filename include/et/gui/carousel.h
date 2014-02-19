/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/element3d.h>

namespace et
{
	namespace gui
	{

		class Carousel;

		enum CarouselType
		{
			CarouselType_Ribbon,
			CarouselType_Round,
			CarouselType_max
		};

		class CarouselItem : public Element3D 
		{
		public:
			typedef IntrusivePtr<CarouselItem> Pointer;

		public:
			ET_DECLARE_EVENT1(selected, CarouselItem::Pointer)

			float currentAngle() const 
				{ return _angle; }

		private:
			friend class Carousel;

			CarouselItem(const Camera& camera, const Texture& texture, const ImageDescriptor& desc, 
				int tag, Carousel* parent);

			void addToRenderQueue(RenderContext* rc, GuiRenderer& gr);
			void setScale(const vec2& s);
			void setColor(const vec4& color);

			void setAngle(float a)
				{ _angle = a; }

			void setActualIndex(float i)
				{ _actualIndex = i; }

			const vec4& color() const
				{ return _color; }

			bool containsPoint(const vec2&, const vec2&);
			void buildVertexList(GuiRenderer& gr);
			bool rayIntersect(const ray3d& r);

		private:
			vec2 origin() const
				{ return vec2(0.0f); }

			const vec2& size() const 
				{ return _scale; }
			
			vec2 contentSize()
				{ return vec2(0.0f); }

		private:
			GuiVertexList _vertices;
			Texture _texture;
			ImageDescriptor _desc;
			vec2 _scale;
			vec4 _color;
			float _angle;
			float _actualIndex;
		};

		typedef std::list<CarouselItem::Pointer> CarouselItemList;

		class Carousel : public Element3D
		{
		public:
			typedef IntrusivePtr<Carousel> Pointer;

		public:
			Carousel(const Camera& camera, Element* parent);
			~Carousel();

			CarouselItem::Pointer addItem(int tag, const Texture& tex, const ImageDescriptor& desc);
			CarouselItem::Pointer appendItem(int tag, const Texture& tex, const ImageDescriptor& desc);
			CarouselItem::Pointer prependItem(int tag, const Texture& tex, const ImageDescriptor& desc);
			CarouselItem::Pointer insertItemAtIndex(int index, int tag, const Texture& tex, const ImageDescriptor& desc);
			CarouselItem::Pointer addItem(int tag, const Image& desc);

			void clear();

			void setScale(const vec2& s);

			void setSelectedItem(size_t item, float duration);

			size_t selectedItem() const;

			void setDragOnlyItems(bool value);

			void setDirection(const vec2& d);

			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerScrolled(const PointerInputInfo&);
			void pointerEntered(const PointerInputInfo&);
			void pointerLeaved(const PointerInputInfo&);

			bool containsPoint(const vec2&, const vec2&);
			void update(float t);

			void setCenter(const vec2& c, float duration = 0.0f);
			void setAlpha(float value, float duration = 0.0f);
				
			virtual bool visible() const
				{ return _alpha > 0.0f; }

			ET_DECLARE_EVENT1(didSelectItem, CarouselItem::Pointer)
			ET_DECLARE_EVENT1(itemClicked, CarouselItem::Pointer)

		private:
			void addToRenderQueue(RenderContext* rc, GuiRenderer& gr);

			void buildItems();
			void buildRibbonItems();
			void buildRoundItems();
			void sortItems();

			void setTransform(const mat4& t)
				{ Element3D::setTransform(t); }

			void applyTransform(const mat4& t)
				{ Element3D::applyTransform(t); }

			void alignSelectedItem(bool fromUpdate);
			bool performClick(const PointerInputInfo& p);

			CarouselItem::Pointer itemForInputInfo(const PointerInputInfo& p, size_t& index);

			vec2 origin() const
				{ return vec2(0.0f); }

			const vec2& size() const
				{ return _scale; }

		private:
			CarouselItemList _items;
			CarouselItemList _sortedItems;

			PointerInputInfo _lastTouch;

			FloatAnimator _setItemAnimator;
			FloatAnimator _alphaAnimator;
			Vector2Animator _positionAnimator;

			float _selectedItem;
			float _lastUpdateTime;
			float _velocity;
			float _clickTime;
			float _alpha;

			vec2 _center;
			vec2 _scale;
			vec2 _direction;

			CarouselType _type;
			bool _pointerPressed;
			bool _dragging;
			bool _animating;
			bool _waitingClick;
			bool _dragOnlyItems;
		};
	}
}