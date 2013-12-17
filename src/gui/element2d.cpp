/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/geometry/geometry.h>
#include <et/gui/element2d.h>

using namespace et;
using namespace et::gui;

ET_DECLARE_GUI_ELEMENT_CLASS(Element2d)

Element2d::Element2d(Element* parent, const std::string& name) :
	Element(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _frameAnimator(timerPool()),
	_colorAnimator(timerPool()), _scaleAnimator(timerPool()), _angleAnimator(timerPool()),
	_frame(0.0f, 0.0f, 0.0f, 0.0f), _scale(1.0f), _color(1.0f), _angle(0.0f), _pivotPoint(0.0f)
{
	_frameAnimator.setTag(AnimatedProperty_Frame);
	_frameAnimator.setDelegate(this);
	_colorAnimator.setTag(AnimatedProperty_Color);
	_colorAnimator.setDelegate(this);
	_scaleAnimator.setTag(AnimatedProperty_Scale);
	_scaleAnimator.setDelegate(this);
	_angleAnimator.setTag(AnimatedProperty_Angle);
	_angleAnimator.setDelegate(this);
}

Element2d::Element2d(const rect& frame, Element* parent, const std::string& name) :
	Element(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _frameAnimator(timerPool()),
	_colorAnimator(timerPool()), _scaleAnimator(timerPool()), _angleAnimator(timerPool()),
	_frame(frame), _scale(1.0f), _color(1.0f), _angle(0.0f), _pivotPoint(0.0f)
{
	_frameAnimator.setTag(AnimatedProperty_Frame);
	_frameAnimator.setDelegate(this);
	_colorAnimator.setTag(AnimatedProperty_Color);
	_colorAnimator.setDelegate(this);
	_scaleAnimator.setTag(AnimatedProperty_Scale);
	_scaleAnimator.setDelegate(this);
	_angleAnimator.setTag(AnimatedProperty_Angle);
	_angleAnimator.setDelegate(this);
}

Element2d::~Element2d()
{
}

const vec2& Element2d::size() const
{ 
	return _frame.size(); 
}

const vec4 Element2d::color() const
{
	return vec4(_color.xyz(), finalAlpha());
}

const vec2& Element2d::position() const
{
	return _frame.origin(); 
}

const rect& Element2d::frame() const
{
	return _frame;
}

const vec2& Element2d::pivotPoint() const
{
	return _pivotPoint;
}

const vec2& Element2d::scale() const
{
	return _scale;
}

float Element2d::angle() const
{
	return _angle;
}

float Element2d::alpha() const
{
	return finalAlpha();
}

bool Element2d::visible() const
{
	return (_color.w > 0.0f) && (_scale.square() != 0.0f);
}

void Element2d::setAngle(float angle, float duration)
{
	if (duration == 0.0f)
	{
		_angle = angle;
		invalidateTransform();
	}
	else 
	{
		_angleAnimator.animate(&_angle, _angle, angle, duration);
	}
}

void Element2d::rotate(float angle, float duration)
{
	return setAngle(_angle + angle, duration);
}

void Element2d::setScale(const vec2& scale, float duration)
{
	if (duration == 0.0f)
	{
		_scale = scale;
		invalidateTransform();
	}
	else 
	{
		_scaleAnimator.animate( &_scale, _scale, scale, duration);
	}
}


void Element2d::setColor(const vec4& color, float duration) 
{ 
	if (duration == 0.0f)
	{
		_color = color; 
		invalidateContent(); 
	}
	else 
	{
		_colorAnimator.animate(&_color, _color, color, duration);
	}
}

void Element2d::setAlpha(const float alpha, float duration) 
{ 
	if ((_color.w == alpha) && !_colorAnimator.running()) return;

	if (duration == 0.0f)
	{
		_color.w = alpha;
		_colorAnimator.cancelUpdates();
		invalidateContent();
	}
	else 
	{
		_colorAnimator.animate(&_color, _color, vec4(_color.xyz(), alpha), duration);
	}
}

void Element2d::setFrame(const rect& r, float duration)
{
	if (duration <= std::numeric_limits<float>::epsilon())
	{
		_frame = r;
		_frameAnimator.cancelUpdates();
		invalidateTransform();
		invalidateContent(); 
	}
	else 
	{
		_frameAnimator.animate(&_frame, _frame, r, duration);
	}
}

void Element2d::setFrame(float x, float y, float width, float height, float duration)
{
	return setFrame(rect(x, y, width, height), duration);
}

void Element2d::setFrame(const vec2& origin, const vec2& size, float duration)
{
	return setFrame(rect(origin, size), duration);
}

void Element2d::setPosition(const vec2& p, float duration) 
{ 
	return setFrame(p.x, p.y, _frame.width, _frame.height, duration);
}

void Element2d::setSize(const vec2& s, float duration) 
{ 
	return setFrame(_frame.left, _frame.top, s.x, s.y, duration);
}

void Element2d::setPosition(float x, float y, float duration) 
{ 
	return setPosition(vec2(x, y), duration); 
}

void Element2d::setSize(float w, float h, float duration) 
{ 
	return setSize(vec2(w, h), duration); 
}

void Element2d::setVisible(bool vis, float duration)
{
	if ((visible() != vis) || _colorAnimator.running())
		setAlpha(vis ? 1.0f : 0.0f, duration);
}

const mat4& Element2d::finalTransform()
{
	if (!transformValid())
		buildFinalTransform();

	return _finalTransform;
}

void Element2d::buildFinalTransform()
{ 
	_finalTransform = translationMatrix(vec3(offset(), 0.0f)) * 
		transform2DMatrix(_angle, _scale, _frame.origin()) * parentFinalTransform(); 

	setTransformValid(true);
}

float Element2d::finalAlpha() const
{
	return parent() ? parent()->finalAlpha() * _color.w : _color.w;
}

void Element2d::animatorUpdated(BaseAnimator* a)
{
	AnimatedPropery prop = static_cast<AnimatedPropery>(a->tag());

	bool isFrame = (prop & AnimatedProperty_Frame) == AnimatedProperty_Frame;
	bool isAngle = (prop & AnimatedProperty_Angle) == AnimatedProperty_Angle;
	bool isScale = (prop & AnimatedProperty_Scale) == AnimatedProperty_Scale;
	bool isColor = (prop & AnimatedProperty_Color) == AnimatedProperty_Color;

	if (isFrame || isAngle || isScale)
		invalidateTransform();

	if (isFrame || isColor)
		invalidateContent();
}

void Element2d::animatorFinished(BaseAnimator* a)
{
	elementAnimationFinished.invoke(this, static_cast<AnimatedPropery>(a->tag()));
}

bool Element2d::containsPoint(const vec2& p, const vec2&) 
{
	return containLocalPoint(finalInverseTransform() * p);
}

bool Element2d::containLocalPoint(const vec2& p)
{
	return (p.x >= 0.0f) && (p.y >= 0.0f) && (p.x < _frame.width) && (p.y < _frame.height);
}

void Element2d::setPivotPoint(const vec2& p, bool preservePosition)
{
	if (_pivotPoint == p) return;

	_pivotPoint = p;

	if (preservePosition)
		setPosition(_frame.origin() - offset());
}

vec2 Element2d::offset() const
{
	return -_frame.size() * _pivotPoint;
}

vec2 Element2d::origin() const
{
	return _frame.origin() + offset();
}

const mat4& Element2d::finalInverseTransform()
{
	if (!inverseTransformValid())
	{
		_finalInverseTransform = finalTransform().inverse();
		setIverseTransformValid(true);
	}

	return _finalInverseTransform;
}

vec2 Element2d::positionInElement(const vec2& p)
{
	return finalInverseTransform() * p;
}

vec2 Element2d::contentSize()
{
	return _frame.size();
}
