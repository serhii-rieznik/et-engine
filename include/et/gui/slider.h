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
        class Slider : public Element2d
        {
        public:
            typedef IntrusivePtr<Slider> Pointer;
			
		public:
			Slider(Element2d* parent);
			
			void setBackgroundImage(const Image&);
			void setBackgroundColor(const vec4&);
			
			void setHandleImage(const Image&, float scale);
			void setSliderImages(const Image& left, const Image& right);
			
			float minValue() const
				{ return _min; }
			
			float maxValue() const
				{ return _max; }

			void setRange(float aMin, float aMax);
			
			void setValue(float v);
			float value() const;
			
			ET_DECLARE_EVENT1(changed, Slider*)
			ET_DECLARE_EVENT1(valueChanged, float)
			
        private:
			void addToRenderQueue(RenderContext*, GuiRenderer&);
			void buildVertices(RenderContext*, GuiRenderer& renderer);
			
			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerCancelled(const PointerInputInfo&);
			
			void updateValue(float);
			
        private:
			Image _background;
			Image _sliderLeft;
			Image _sliderRight;
			Image _handle;
			
			GuiVertexList _backgroundVertices;
			GuiVertexList _sliderLeftVertices;
			GuiVertexList _sliderRightVertices;
			GuiVertexList _handleVertices;
			
			vec4 _backgroundColor;
			
			float _handleScale;
			
			float _min;
			float _max;
			float _value;
			
			bool _drag;
        };
    }
}
