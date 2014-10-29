/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/gui/scroll.h>
#include <et/gui/guirenderer.h>

using namespace et;
using namespace et::gui;

float deccelerationRate = 10.0f;
float accelerationRate = 0.5f;
float scrollbarSize = 5.0f;
float maxScrollbarsVisibilityVelocity = 50.0f;
float minAlpha = 1.0f / 255.0f;
float alphaAnimationScale = 5.0f;
float bounceStopTreshold = 0.5f;

ET_DECLARE_GUI_ELEMENT_CLASS(Scroll)

Scroll::Scroll(Element2d* parent, const std::string& name) :
	Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _offsetAnimator(mainTimerPool()),
	_updateTime(0.0f), _scrollbarsAlpha(0.0f), _scrollbarsAlphaTarget(0.0f), _bounce(0),
	_pointerCaptured(false), _manualScrolling(false)
{
	_offsetAnimator.updated.connect([this]()
		{ setOffsetDirectly(_offset); });
	
	setFlag(Flag_HandlesChildEvents);
	setFlag(Flag_ClipToBounds);
	startUpdates();
}

void Scroll::addToRenderQueue(RenderContext* rc, GuiRenderer& r)
{
	if (!contentValid())
		buildVertices(rc, r);
	
	r.addVertices(_backgroundVertices, Texture(), ElementRepresentation_2d, RenderLayer_Layer0);
}

void Scroll::addToOverlayRenderQueue(RenderContext* rc, GuiRenderer& r)
{
	if (!contentValid())
		buildVertices(rc, r);

	r.addVertices(_scrollbarsVertices, Texture(), ElementRepresentation_2d, RenderLayer_Layer0);
}

void Scroll::buildVertices(RenderContext* rc, GuiRenderer& r)
{
	_backgroundVertices.setOffset(0);
	_scrollbarsVertices.setOffset(0);

	if (_backgroundColor.w > 0.0f)
	{
		r.createColorVertices(_backgroundVertices, rect(vec2(0.0f), size()), _backgroundColor,
			Element2d::finalTransform());
	}
	
	if (_scrollbarsColor.w > 0.0f)
	{
		float scaledScollbarSize = scrollbarSize * static_cast<float>(rc->screenScaleFactor());
		float barHeight = size().y * (size().y / _contentSize.y);
		float barOffset = size().y * (_offset.y / _contentSize.y);
		vec2 origin(size().x - 2.0f * scaledScollbarSize, -barOffset);
		
		vec4 adjutsedColor = _scrollbarsColor;
		adjutsedColor.w *= _scrollbarsAlpha;
		
		r.createColorVertices(_scrollbarsVertices, rect(origin, vec2(scaledScollbarSize, barHeight)), adjutsedColor,
		  Element2d::finalTransform());
	}
	
	setContentValid();
}

const mat4& Scroll::finalTransform()
{
	_localFinalTransform = Element2d::finalTransform();
	_localFinalTransform[3] += vec4(_offset, 0.0f, 0.0f);
	return _localFinalTransform;
}

const mat4& Scroll::finalInverseTransform()
{
	_localInverseTransform = Element2d::finalTransform().inverse();
	return _localInverseTransform;
}

bool Scroll::pointerPressed(const PointerInputInfo& p)
{
	if ((_currentPointer.id == 0) && (p.type == PointerType_General))
	{
		_previousPointer = p;
		_currentPointer = p;
		_manualScrolling = false;
		_pointerCaptured = false;
		_velocity = vec2(0.0f);
	}

	broadcastPressed(p);
	return true;
}

bool Scroll::pointerMoved(const PointerInputInfo& p)
{
	if (p.id != _currentPointer.id) return true;

	vec2 offset = p.pos - _currentPointer.pos;
	if (offset.dotSelf() < SQRT_2) return true;

	if (_manualScrolling)
	{
		vec2 offsetScale(1.0f);

		if (-_offset.x < scrollLeftDefaultValue())
		{
			float diff = std::abs(-_offset.x - scrollLeftDefaultValue());
			offsetScale.x *= etMax(0.0f, 1.0f - diff / scrollOutOfContentXSize());
		}
		else if (-_offset.x > scrollRightDefaultValue())
		{
			float diff = std::abs(-_offset.x - scrollRightDefaultValue());
			offsetScale.x *= etMax(0.0f, 1.0f - diff / scrollOutOfContentXSize());
		}
		if (-_offset.y < scrollUpperDefaultValue())
		{
			float diff = std::abs(-_offset.y - scrollUpperDefaultValue());
			offsetScale.y *= etMax(0.0f, 1.0f - diff / scrollOutOfContentYSize());
		}
		else if (-_offset.y > scrollLowerDefaultValue())
		{
			float diff = std::abs(-_offset.y - scrollLowerDefaultValue());
			offsetScale.y *= etMax(0.0f, 1.0f - diff / scrollOutOfContentYSize());
		}
		
		applyOffset(sqr(offsetScale) * offset);
	}
	else if (_selectedElement.valid() && _selectedElement->capturesPointer())
	{
		broadcastMoved(p);
	}
	else if (!_pointerCaptured && (p.type == PointerType_General))
	{
		_manualScrolling = true;
		_pointerCaptured = true;
		_scrollbarsAlphaTarget = 1.0f;
		_bouncing = vector2<BounceDirection>(BounceDirection_None);
		broadcastCancelled(p);
	}
	
	_previousPointer = _currentPointer;
	_currentPointer = p;

	return true;
}

bool Scroll::pointerReleased(const PointerInputInfo& p)
{
	if (p.id == _currentPointer.id)
	{
		if (!_pointerCaptured)
			broadcastReleased(p);

		_pointerCaptured = false;
		_manualScrolling = false;
		_currentPointer = PointerInputInfo();
		_previousPointer = _currentPointer;
	}
	else 
	{
		broadcastReleased(p);
	}

	return true;
}

bool Scroll::pointerCancelled(const PointerInputInfo& p)
{
	_pointerCaptured = false;
	broadcastCancelled(p);
	return true;
}

void Scroll::invalidateChildren()
{
	for (auto& c : children())
	{
		c->invalidateTransform();
		c->invalidateContent();
	}
}

void Scroll::broadcastPressed(const PointerInputInfo& p)
{
	PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos,
		p.scroll, p.id, p.timestamp, p.origin);

	for (auto i = children().rbegin(), e = children().rend(); i != e; ++i)
	{
		Element* el = i->ptr();
		if (el->enabled() && el->visible() && el->containsPoint(globalPos.pos, globalPos.normalizedPos))
		{
			vec2 posInElement = el->positionInElement(globalPos.pos);
			if (el->pointerPressed(PointerInputInfo(p.type, posInElement, globalPos.normalizedPos,
				p.scroll, p.id, p.timestamp, p.origin)))
			{
				_selectedElement = Element::Pointer(el);
				break;
			}
		}
	}
}

void Scroll::broadcastMoved(const PointerInputInfo& p)
{
	PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll,
		p.id, p.timestamp, p.origin);

	for (auto i = children().rbegin(), e = children().rend(); i != e; ++i)
	{
		Element* el = i->ptr();
		if (el-visible() && el->enabled())
		{
			el->pointerMoved(PointerInputInfo(p.type, el->positionInElement(globalPos.pos),
				globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		}
	}
}

void Scroll::broadcastReleased(const PointerInputInfo& p)
{
	PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll,
		p.id, p.timestamp, p.origin);

	for (auto i = children().rbegin(), e = children().rend(); i != e; ++i)
	{
		Element* el = i->ptr();
		if (el-visible() && el->enabled())
		{
			el->pointerReleased(PointerInputInfo(p.type, el->positionInElement(globalPos.pos),
				globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		}
	}
	
	_selectedElement.reset(0);
}

void Scroll::broadcastCancelled(const PointerInputInfo& p)
{
	PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll,
		p.id, p.timestamp, p.origin);

	for (auto i = children().rbegin(), e = children().rend(); i != e; ++i)
	{
		Element* el = i->ptr();
		if (el-visible() && el->enabled())
		{
			el->pointerCancelled(PointerInputInfo(p.type, el->positionInElement(globalPos.pos),
				globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		}
	}
	
	_selectedElement.reset(0);
}

bool Scroll::containsPoint(const vec2& p, const vec2& np)
{
	return Element2d::containsPoint(p, np);
}

void Scroll::updateBouncing(float deltaTime)
{
	if (-_offset.x < scrollLeftDefaultValue())
		_bouncing.x = BounceDirection_ToNear;
	if (-_offset.x > scrollRightDefaultValue())
		_bouncing.x = BounceDirection_ToFar;
	if (-_offset.y < scrollUpperDefaultValue())
		_bouncing.y = BounceDirection_ToNear;
	if (-_offset.y > scrollLowerDefaultValue())
		_bouncing.y = BounceDirection_ToFar;

	if (_bouncing.x == BounceDirection_ToNear)
	{
		float diff = -_offset.x - scrollLeftDefaultValue();
		_velocity.x += 0.25f * size().x * diff * deltaTime;
		if ((_velocity.x <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold))
		{
			_velocity.x = 0.0f;
			_offset.x = -scrollLeftDefaultValue();
			_bouncing.x = BounceDirection_None;
		}
	}
	else if (_bouncing.x == BounceDirection_ToFar)
	{
		float diff = -_offset.x - scrollRightDefaultValue();
		_velocity.x += 0.25f * size().x * diff * deltaTime;
		if ((_velocity.x <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold))
		{
			_velocity.x = 0.0f;
			_offset.x = -scrollRightDefaultValue();
			_bouncing.x = BounceDirection_None;
		}
	}

	if (_bouncing.y == BounceDirection_ToNear)
	{
		float diff = -_offset.y - scrollUpperDefaultValue();
		_velocity.y += 0.25f * size().y * diff * deltaTime;
		if ((_velocity.y <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold))
		{
			_velocity.y = 0.0f;
			_offset.y = -scrollUpperDefaultValue();
			_bouncing.y = BounceDirection_None;
		}
	}
	else if (_bouncing.y == BounceDirection_ToFar)
	{
		float diff = -_offset.y - scrollLowerDefaultValue();
		_velocity.y += 0.25f * size().y * diff * deltaTime;
		if ((_velocity.y <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold))
		{
			_velocity.y = 0.0f;
			_offset.y = -scrollLowerDefaultValue();
			_bouncing.y = BounceDirection_None;
		}
	}
}

void Scroll::update(float t)
{
	if (_updateTime == 0.0f)
		_updateTime = t;
	
	float deltaTime = t - _updateTime;
	
	_updateTime = t;
	
	_scrollbarsAlpha =
		mix(_scrollbarsAlpha, _scrollbarsAlphaTarget, etMin(1.0f, alphaAnimationScale * deltaTime));
	
	if (_scrollbarsAlpha < minAlpha)
		_scrollbarsAlpha = 0.0f;

	if (_manualScrolling)
	{
		_scrollbarsAlphaTarget = 1.0f;
		float dt = _currentPointer.timestamp - _previousPointer.timestamp;
		if (dt > 1.0e-2)
		{
			vec2 dp = _currentPointer.pos - _previousPointer.pos;
			_velocity = mix(_velocity, dp * (accelerationRate / dt), 0.5f);
		}
		invalidateContent();
		return;
	}

	updateBouncing(deltaTime);
	
	float dt = etMin(1.0f, deltaTime * deccelerationRate);
	_velocity *= 1.0f - dt;
	
	_scrollbarsAlphaTarget = etMin(1.0f, _velocity.dotSelf() / maxScrollbarsVisibilityVelocity);

	if (_velocity.dotSelf() < 1.0f)
		_velocity = vec2(0.0f);

	vec2 dp = _velocity * deltaTime;
	
	if (dp.dotSelf() > 1.0e-6)
	{
		applyOffset(dp);
	}
	else if (std::abs(_scrollbarsAlpha - _scrollbarsAlphaTarget) > minAlpha)
	{
		invalidateContent();
	}
}

void Scroll::setContentSize(const vec2& cs)
{
	_contentSize = cs;
	invalidateContent();
}

void Scroll::adjustContentSize()
{
	vec2 size;
	
	for (auto& ptr : children())
	{
		if (ptr->visible())
			size = maxv(size, ptr->origin() + ptr->size());
	}

	setContentSize(size);
}

void Scroll::applyOffset(const vec2& dOffset, float duration)
{
	setOffset(_offset + dOffset, duration);
}

void Scroll::setOffset(const vec2& aOffset, float duration)
{
	_offsetAnimator.cancelUpdates();
	
	if (duration == 0.0f)
	{
		_offsetAnimator.cancelUpdates();
		setOffsetDirectly(aOffset);
	}
	else
	{
		_offsetAnimator.animate(&_offset, _offset, aOffset, duration);
	}
}

float Scroll::scrollOutOfContentXSize() const
	{ return horizontalBounce() ? 0.5f * size().x : 0.001f; }

float Scroll::scrollOutOfContentYSize() const
	{ return verticalBounce() ? 0.5f * size().y : 0.001f; }

float Scroll::scrollUpperDefaultValue() const
	{ return 0.0f; }

float Scroll::scrollLowerDefaultValue() const
	{ return etMax(0.0f, _contentSize.y - size().y); }

float Scroll::scrollLeftDefaultValue() const
	{ return 0.0f; }

float Scroll::scrollRightDefaultValue() const
	{ return etMax(0.0f, _contentSize.x - size().x); }

float Scroll::scrollUpperLimit() const
	{ return scrollUpperDefaultValue() - scrollOutOfContentYSize(); }

float Scroll::scrollLowerLimit() const
	{ return scrollLowerDefaultValue() + scrollOutOfContentYSize(); }

float Scroll::scrollLeftLimit() const
	{ return scrollLeftDefaultValue() - scrollOutOfContentXSize(); }

float Scroll::scrollRightLimit() const
	{ return scrollRightDefaultValue() + scrollOutOfContentXSize(); }

void Scroll::setOffsetDirectly(const vec2& o)
{
	_offset = o;
	vec2 actualOffset = -_offset;
	
	float leftLimit = scrollLeftLimit();
	float rightLimit = scrollRightLimit();
	float upperLimit = scrollUpperLimit();
	float lowerLimit = scrollLowerLimit();

	if (actualOffset.x < leftLimit)
	{
		_offset.x = -leftLimit;
		_velocity.x = 0.0f;
	}
	
	if (actualOffset.x > rightLimit)
	{
		_offset.x = -rightLimit;
		_velocity.x = 0.0f;
	}

	if (actualOffset.y < upperLimit)
	{
		_offset.y = -upperLimit;
		_velocity.y = 0.0f;
	}
	
	if (actualOffset.y > lowerLimit)
	{
		_offset.y = -lowerLimit;
		_velocity.y = 0.0f;
	}
	
	invalidateContent();
	invalidateChildren();
}

void Scroll::setBackgroundColor(const vec4& color)
{
	_backgroundColor = color;
	invalidateContent();
}

void Scroll::setScrollbarsColor(const vec4& c)
{
	_scrollbarsColor = c;
	invalidateContent();
}

void Scroll::setBounce(size_t b)
{
	_bounce = b;
}

void Scroll::scrollToBottom(float delay)
{
	setOffset(vec2(_offset.x, etMin(0.0f, size().y - _contentSize.y)), delay);
}

vec2 Scroll::contentSize()
{
	adjustContentSize();
	return _contentSize;
}
