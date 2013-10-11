/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/gui/guirenderer.h>
#include <et/gui/button.h>

using namespace et;
using namespace et::gui;

ET_DECLARE_GUI_ELEMENT_CLASS(Button)

Button::Button(const std::string& title, Font font, Element2d* parent, const std::string& name) :
	Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _title(title), _font(font),
	_textSize(font->measureStringSize(title, true)), _textColor(vec3(0.0f), 1.0f),
	_textPressedColor(vec3(0.0f), 1.0f), _type(Button::Type_PushButton), _state(State_Default),
	_imageLayout(ImageLayout_Left), _contentMode(ContentMode_Fit), _pressed(false),
	_hovered(false), _selected(false)
{
	setSize(sizeForText(title));
}

void Button::addToRenderQueue(RenderContext* rc, GuiRenderer& gr)
{
	if (!contentValid() || !transformValid())
		buildVertices(rc, gr);

	if (_bgVertices.lastElementIndex() > 0)
		gr.addVertices(_bgVertices, _background[_state].texture, ElementRepresentation_2d, RenderLayer_Layer0);

	if (_textVertices.lastElementIndex() > 0)
		gr.addVertices(_textVertices, _font->texture(), ElementRepresentation_2d, RenderLayer_Layer1);

	if (_imageVertices.lastElementIndex() > 0)
		gr.addVertices(_imageVertices, _image.texture, ElementRepresentation_2d, RenderLayer_Layer0);
}

void Button::buildVertices(RenderContext*, GuiRenderer& gr)
{
	mat4 transform = finalTransform();
	
	vec2 frameSize = size() + _contentOffset;
	vec2 imageSize = absv(_image.descriptor.size);
	
	if (_contentMode == ContentMode_Fit)
	{
		float imageAspect = imageSize.aspect();
		if (imageSize.x > frameSize.x)
		{
			imageSize.x = frameSize.x;
			imageSize.y = frameSize.x / imageAspect;
		}
		if (imageSize.y > frameSize.y)
		{
			imageSize.x = frameSize.y * imageAspect;
			imageSize.y = frameSize.y;
		}
	}
	else if (_contentMode == ContentMode_ScaleMaxToMin)
	{
		float maxImageDim = etMax(imageSize.x, imageSize.y);
		float minFrameDim = etMin(frameSize.x, frameSize.y);
		imageSize *= minFrameDim / maxImageDim;
	}
	else
	{
		assert("Uknown content mode" && 0);
	}
	
	float contentGap = (imageSize.x > 0.0f) && (_textSize.x > 0.0f) ? 5.0f : 0.0f;
	float contentWidth = imageSize.x + _textSize.x + contentGap;

	vec2 imageOrigin;
	vec2 textOrigin;
	
	if (_imageLayout == ImageLayout_Right)
	{
		textOrigin = 0.5f * (frameSize - vec2(contentWidth, _textSize.y));
		imageOrigin = vec2(textOrigin.x + contentGap + _textSize.x, 0.5f * (frameSize.y - imageSize.y));
	}
	else
	{
		imageOrigin = 0.5f * (frameSize - vec2(contentWidth, imageSize.y));
		textOrigin = vec2(imageOrigin.x + contentGap + imageSize.x, 0.5f * (frameSize.y - _textSize.y));
	}
	vec4 alphaScale = vec4(1.0f, 1.0f, 1.0f, alpha());
	
	_bgVertices.setOffset(0);
	_textVertices.setOffset(0);
	_imageVertices.setOffset(0);
	
	if (_backgroundColor.w > 0.0f)
		gr.createColorVertices(_bgVertices, rect(vec2(0.0f), size()), _backgroundColor * alphaScale, transform);

	if (_background[_state].texture.valid())
	{
		gr.createImageVertices(_bgVertices, _background[_state].texture, _background[_state].descriptor,
			rect(vec2(0.0f), size()), color(), transform, RenderLayer_Layer0);
	}

	if (_title.size() > 0)
	{
		vec4 aColor = _state == State_Pressed ? _textPressedColor : _textColor;
		if (aColor.w > 0.0f)
		{
			gr.createStringVertices(_textVertices, _font->buildString(_title, true), Alignment_Near,
				Alignment_Near, textOrigin, aColor * alphaScale, transform, RenderLayer_Layer1);
		}
	}

	if (_image.texture.valid())
	{
		vec4 aColor = _state == State_Pressed ?
			color() * vec4(0.5f, 0.5f, 0.5f, 1.0f) : color();
		
		if (aColor.w > 0.0f)
		{
			gr.createImageVertices(_imageVertices, _image.texture, _image.descriptor, 
				rect(imageOrigin, imageSize), aColor * alphaScale, transform, RenderLayer_Layer0);
		}
	}
}

void Button::setBackgroundForState(const Texture& tex, const ImageDescriptor& desc, State s)
{
	setBackgroundForState(Image(tex, desc), s);
}

void Button::setBackgroundForState(const Image& img, State s)
{
	_background[s] = img;
	invalidateContent();
}

bool Button::pointerPressed(const PointerInputInfo& p)
{
	if (p.type != PointerType_General) return false;

	_pressed = true;
	setCurrentState(_selected ? State_SelectedPressed : State_Pressed);
	
	pressed.invoke(this);
	return true;
}

bool Button::pointerReleased(const PointerInputInfo& p)
{
	if ((p.type != PointerType_General) || !_pressed) return false;
	
	_pressed = false;
	State newState = _selected ? State_Selected : State_Default;

	if (containLocalPoint(p.pos))
	{
		performClick();
		newState = adjustState(_selected ? State_SelectedHovered : State_Hovered);
	}

	setCurrentState(newState);
	released.invoke(this);
	return true;
}

bool Button::pointerCancelled(const PointerInputInfo& p)
{
	if ((p.type != PointerType_General) || !_pressed) return false;
	
	_pressed = false;
	State newState = newState = _selected ? State_Selected : State_Default;
	
	if (containLocalPoint(p.pos))
		newState = adjustState(_selected ? State_SelectedHovered : State_Hovered);
	
	setCurrentState(newState);
	released.invoke(this);
	return true;
}

void Button::pointerEntered(const PointerInputInfo&)
{
	if (_selected)
		setCurrentState(adjustState(_pressed ? State_SelectedPressed : State_SelectedHovered));
	else
		setCurrentState(adjustState(_pressed ? State_Pressed : State_Hovered));
}

void Button::pointerLeaved(const PointerInputInfo&)
{
	if (_selected)
		setCurrentState(_pressed ? State_SelectedPressed : State_Selected);
	else
		setCurrentState(_pressed ? State_Pressed : State_Default);
}

void Button::setCurrentState(State s)
{
	if (_state == s) return;

	_state = s;
	invalidateContent();
}

bool Button::capturePointer() const
{
	return true;
}

void Button::performClick()
{
	if (_type == Type_CheckButton)
		setSelected(!_selected);
	
	clicked.invoke(this);
}

const vec2& Button::textSize()
{
	return _textSize; 
}

void Button::setTitle(const std::string& t)
{
	if (_title == t) return;

	_title = t;
	_textSize = _font->measureStringSize(_title, true);
	invalidateContent();
}

void Button::setTextColor(const vec4& color)
{
	_textColor = color;
	invalidateContent();
}

const vec4& Button::textColor() const
{
	return _textColor;
}

void Button::setTextPressedColor(const vec4& color)
{
	_textPressedColor = color;
	invalidateContent();
}

const vec4& Button::textPressedColor() const
{
	return _textPressedColor;
}

void Button::adjustSize(float duration)
{
	adjustSizeForText(_title, duration);
}

void Button::adjustSizeForText(const std::string& text, float duration)
{
	setSize(sizeForText(text), duration);
}

vec2 Button::sizeForText(const std::string& text)
{
	vec2 textSize = _font.valid() ? _font->measureStringSize("AA" + text + "AA", true) : vec2(0.0f);
	
	for (size_t i = 0; i < State_max; ++i)
		textSize = maxv(textSize, _background[i].descriptor.size);

	return vec2(floorf(textSize.x), floorf(1.25f * textSize.y));
}

void Button::setImage(const Image& img)
{
	_image = img;
	invalidateContent();
}

void Button::setSelected(bool s)
{
	bool wasSelected = _selected;
	_selected = s && (_type == Type_CheckButton);

	if (wasSelected != _selected)
	{
		if (elementIsSelected(_state))
			setCurrentState(static_cast<State>(_state - 3 * static_cast<int>(!_selected)));
		else
			setCurrentState(static_cast<State>(_state + 3 * static_cast<int>(_selected)));
	}
}

void Button::setType(Button::Type t)
{
	_type = t;
	setSelected(false);
}

void Button::setContentOffset(const vec2& o)
{
	_contentOffset = o;
	invalidateContent();
}

void Button::setImageLayout(ImageLayout l)
{
	_imageLayout = l;
	invalidateContent();
}

void Button::setBackgroundColor(const vec4& color)
{
	_backgroundColor = color;
	invalidateContent();
}

const vec4& Button::backgroundColor() const
{
	return _backgroundColor;
}

void Button::setContentMode(ContentMode m)
{
	_contentMode = m;
	invalidateContent();
}

vec2 Button::contentSize()
{
	return sizeForText(_title);
}
