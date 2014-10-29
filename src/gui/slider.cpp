/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Additional thanks to Kirill Polezhaiev
 * Please, do not modify content without approval.
 *
 */

#include <et/gui/guirenderer.h>
#include <et/gui/slider.h>

using namespace et;
using namespace et::gui;

Slider::Slider(Element2d* parent) :
	Element2d(parent), _handleScale(1.0f), _min(0.0f), _max(1.0f), _value(0.5f), _drag(false)
{
	
}

void Slider::setRange(float aMin, float aMax)
{
	float oldValue = value();
	
	_min = aMin;
	_max = aMax;
	
	if (oldValue < _min)
		setValue(_min);
	
	if (oldValue > _max)
		setValue(_max);
	
	invalidateContent();
}

void Slider::setValue(float v)
{
	_value = (v - _min) / (_max - _min);
	invalidateContent();
}

float Slider::value() const
{
	return mix(_min, _max, _value);
}

void Slider::addToRenderQueue(RenderContext* rc, GuiRenderer& guiRenderer)
{
	if (!contentValid() || !transformValid())
		buildVertices(rc, guiRenderer);

	if (_backgroundVertices.lastElementIndex() > 0)
		guiRenderer.addVertices(_backgroundVertices, _background.texture, ElementRepresentation_2d, RenderLayer_Layer0);

	if (_sliderLeftVertices.lastElementIndex() > 0)
		guiRenderer.addVertices(_sliderLeftVertices, _sliderLeft.texture, ElementRepresentation_2d, RenderLayer_Layer0);

	if (_sliderRightVertices.lastElementIndex() > 0)
		guiRenderer.addVertices(_sliderRightVertices, _sliderRight.texture, ElementRepresentation_2d, RenderLayer_Layer0);
	
	if (_handleVertices.lastElementIndex() > 0)
		guiRenderer.addVertices(_handleVertices, _handle.texture, ElementRepresentation_2d, RenderLayer_Layer0);
}

void Slider::buildVertices(RenderContext*, GuiRenderer& renderer)
{
	mat4 transform = finalTransform();
	rect mainRect(vec2(0.0f), size());
	
	_backgroundVertices.setOffset(0);
	_sliderLeftVertices.setOffset(0);
	_sliderRightVertices.setOffset(0);
	_handleVertices.setOffset(0);
	
	float handleWidth = _handle.descriptor.size.x;
	float halfHandleWidth = 0.5f * handleWidth;
	float valuePoint = _value * mainRect.width;
	
	if (_backgroundColor.w > 0.0f)
		renderer.createColorVertices(_backgroundVertices, mainRect, _backgroundColor, transform);

	if (_background.texture.valid())
	{
		renderer.createImageVertices(_backgroundVertices, _background.texture,
			_background.descriptor, mainRect, vec4(1.0f), transform, RenderLayer_Layer0);
	}
	
	if (_sliderLeft.texture.valid() && (_value > 0.0f))
	{
		rect r(vec2(halfHandleWidth, 0.0f), _sliderLeft.descriptor.size);
		r.top = 0.5f * (mainRect.height - r.height);
		r.width = clamp(valuePoint - halfHandleWidth, 0.0f, mainRect.width - handleWidth);
		
		renderer.createImageVertices(_sliderLeftVertices, _sliderLeft.texture,
			_sliderLeft.descriptor, r, vec4(1.0f), transform, RenderLayer_Layer0);
	}

	if (_sliderRight.texture.valid() && (_value < 1.0f))
	{
		rect r(vec2(0.0f), _sliderRight.descriptor.size);
		r.top = 0.5f * (mainRect.height - r.height);
		r.left = clamp(valuePoint, halfHandleWidth, mainRect.width - halfHandleWidth);
		r.width = etMax(0.0f, mainRect.width - halfHandleWidth - r.left);
		renderer.createImageVertices(_sliderRightVertices, _sliderRight.texture,
			_sliderRight.descriptor, r, vec4(1.0f), transform, RenderLayer_Layer0);
	}

	if (_handle.texture.valid())
	{
		rect r(vec2(0.0f), _handleScale * _handle.descriptor.size);
		r.top = 0.5f * (mainRect.height - r.height);
		r.left = clamp(valuePoint - halfHandleWidth, 0.0f, mainRect.width - handleWidth);
		renderer.createImageVertices(_handleVertices, _handle.texture, _handle.descriptor, r,
			vec4(1.0f), transform, RenderLayer_Layer0);
	}

	setContentValid();
}

void Slider::setBackgroundImage(const Image& i)
{
	_background = i;
	invalidateContent();
}

void Slider::setHandleImage(const Image& i, float scale)
{
	_handle = i;
	_handleScale = scale;
	invalidateContent();
}

void Slider::setSliderImages(const Image& left, const Image& right)
{
	_sliderLeft = left;
	_sliderRight = right;
	invalidateContent();
}

bool Slider::pointerPressed(const PointerInputInfo& p)
{
	_drag = true;
	updateValue(clamp(p.pos.x / size().x, 0.0f, 1.0f));
	return true;
}

bool Slider::pointerMoved(const PointerInputInfo& p)
{
	if (_drag)
	{
		updateValue(clamp(p.pos.x / size().x, 0.0f, 1.0f));
	}
	
	return true;
}

bool Slider::pointerReleased(const PointerInputInfo&)
{
	_drag = false;
	return true;
}

bool Slider::pointerCancelled(const PointerInputInfo&)
{
	_drag = false;
	return true;
}

void Slider::updateValue(float v)
{
	_value = v;
	invalidateContent();
	
	changed.invoke(this);
	valueChanged.invoke(value());
}

void Slider::setBackgroundColor(const vec4& c)
{
	_backgroundColor = c;
	invalidateContent();
}
