/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <algorithm>
#include <et/core/et.h>
#include <et/gui/guirenderer.h>
#include <et/gui/carousel.h>

using namespace et;
using namespace et::gui;

const float carouselItemScale = 1.0f / 3.0f;
const float minUpdateVelocity = 0.01f;
const float minUpdateTime = 0.01f;
const float minUpdateDelta = 0.01f;
const float clickTimeout = std::numeric_limits<float>::infinity();
const float minDragToNextItemVelocity = 1.0f;
const float slowdownCoefficient = 10.0f;
const float movementScale = 1300.0f;
const float maxVelocity = 30.0f;

extern const float defaultAnimationDuration = 0.3f;

bool roundDistanceFunc(const CarouselItem::Pointer& i1, const CarouselItem::Pointer& i2);
bool ribbonDistanceFunc(const CarouselItem::Pointer& i1, const CarouselItem::Pointer& i2);

CarouselItem::CarouselItem(const Camera& camera, const Texture& texture,
	const ImageDescriptor& desc, int aTag, Carousel* parent) : Element3D(camera, parent),
	_vertices(12, 0), _texture(texture), _desc(desc), _scale(1.0f), _color(1.0f), _angle(1.0f)
{
	tag_i = aTag;
	_scale.y *= desc.size.y / desc.size.x;
}

void CarouselItem::setScale(const vec2& s)
{
	_scale = s;
	invalidateContent();
}

void CarouselItem::setColor(const vec4& color)
{
	_color = color;
	invalidateContent();
}

void CarouselItem::addToRenderQueue(RenderContext*, GuiRenderer& gr)
{
	if (!contentValid())
		buildVertexList(gr);

	gr.addVertices(_vertices, _texture, ElementRepresentation_3d, RenderLayer_Layer0);
}

void CarouselItem::buildVertexList(GuiRenderer& gr)
{
	_vertices.setOffset(0);
	if (!_texture.valid()) return;
	
	mat4 transform = finalTransform();

	float indexScale = 0.25f;

	vec3 topLeft = transform * vec3(-carouselItemScale * _scale.x,  carouselItemScale * _scale.y, 0.0f);
	vec3 topMiddle = transform * vec3(0.0f,  carouselItemScale * _scale.y, 0.0f);
	vec3 bottomLeft = transform * vec3(-carouselItemScale * _scale.x, -carouselItemScale * _scale.y, 0.0f);
	vec3 bottomMiddle = transform * vec3(0.0f, -carouselItemScale * _scale.y, 0.0f);
	vec3 topRight = transform * vec3( carouselItemScale * _scale.x,  carouselItemScale * _scale.y, 0.0f);
	vec3 bottomRight = transform * vec3( carouselItemScale * _scale.x, -carouselItemScale * _scale.y, 0.0f);

	vec2 topLeftUV = _desc.origin;
	vec2 bottomLeftUV = _desc.origin + vec2(0.0f, _desc.size.y);
	vec2 topMiddleUV = _desc.origin + vec2(0.5f * _desc.size.x, 0.0f);
	vec2 bottomMiddleUV = _desc.origin + vec2(0.5f * _desc.size.x, _desc.size.y);
	vec2 topRightUV = _desc.origin + vec2(_desc.size.x, 0.0f);
	vec2 bottomRightUV = _desc.origin + _desc.size;

	vec4 leftColor = _color;
	vec4 middleColor = _color;
	vec4 rightColor = _color;
	leftColor.w *= clamp((indexScale - _actualIndex) / indexScale, 0.0f, 1.0f);
	rightColor.w *= clamp((indexScale + _actualIndex) / indexScale, 0.0f, 1.0f);
	middleColor.w *= 0.5f * leftColor.w * rightColor.w + 0.5f;

	vec2 mask(0.0f);

	gr.buildQuad(_vertices,
		GuiVertex(topLeft, vec4(_texture->getTexCoord(topLeftUV), mask), leftColor),
		GuiVertex(topMiddle, vec4(_texture->getTexCoord(topMiddleUV), mask), middleColor), 
		GuiVertex(bottomLeft, vec4(_texture->getTexCoord(bottomLeftUV), mask), leftColor),
		GuiVertex(bottomMiddle,	vec4(_texture->getTexCoord(bottomMiddleUV), mask), middleColor));

	gr.buildQuad(_vertices,
		GuiVertex(topMiddle, vec4(_texture->getTexCoord(topMiddleUV), mask), middleColor),
		GuiVertex(topRight, vec4(_texture->getTexCoord(topRightUV), mask), rightColor), 
		GuiVertex(bottomMiddle,	vec4(_texture->getTexCoord(bottomMiddleUV), mask), middleColor),
		GuiVertex(bottomRight, 	vec4(_texture->getTexCoord(bottomRightUV), mask), rightColor));

	setContentValid();
}

bool CarouselItem::containsPoint(const vec2&, const vec2&)
{
	return false;
}

bool CarouselItem::rayIntersect(const ray3d& r)
{
	if ((std::abs(_color.w) <= 1.0e-4) || !contentValid()) return false;

	for (size_t t = 0; t < _vertices.size(); t += 3)
	{
		triangle tri(_vertices[t].position, _vertices[t+1].position, _vertices[t+2].position);
		if (intersect::rayTriangle(r, tri, 0))
			return true;
	}

	return false;
}

/*
 *
 * Carousel
 *
 */ 
Carousel::Carousel(const Camera& camera, Element* parent) : Element3D(camera, parent), 
	_setItemAnimator(timerPool()), _alphaAnimator(timerPool()), _positionAnimator(timerPool()),
	_selectedItem(0.0f), _lastUpdateTime(0.0f), _scale(1.0f), _direction(1.0f, 0.0f), 
	_velocity(0.0f), _clickTime(0.0f), _alpha(1.0f), _type(CarouselType_Ribbon), 
	_pointerPressed(false), _dragging(false), _animating(false), _waitingClick(false), 
	_dragOnlyItems(false)
{
	_setItemAnimator.setDelegate(this);
	_alphaAnimator.setDelegate(this);
	_positionAnimator.setDelegate(this);

	startUpdates();
	_lastUpdateTime = actualTime();
}

Carousel::~Carousel()
{
	clear();
}

void Carousel::addToRenderQueue(RenderContext* rc, GuiRenderer& gr)
{
	for (auto& i : _sortedItems)
		i->addToRenderQueue(rc, gr);
}

void Carousel::setScale(const vec2& s)
{
	_scale = s;
	for (auto& i : _sortedItems)
		i->setScale(s);
}

CarouselItem::Pointer Carousel::addItem(int tag, const Texture& tex, const ImageDescriptor& desc)
{
	return appendItem(tag, tex, desc);
}

CarouselItem::Pointer Carousel::addItem(int tag, const Image& desc)
{
	return addItem(tag, desc.texture, desc.descriptor);
}

CarouselItem::Pointer Carousel::appendItem(int tag, const Texture& tex,
	const ImageDescriptor& desc)
{
	return insertItemAtIndex(static_cast<int>(_items.size()), tag, tex, desc);
}

CarouselItem::Pointer Carousel::prependItem(int tag, const Texture& tex,
	const ImageDescriptor& desc)
{
	return insertItemAtIndex(0, tag, tex, desc);
}

CarouselItem::Pointer Carousel::insertItemAtIndex(int index, int tag,
	const Texture& tex, const ImageDescriptor& desc)
{
	CarouselItem::Pointer item(new CarouselItem(camera(), tex, desc, tag, this));
	item->setScale(_scale);

	if (index <= 0)
	{
		_items.insert(_items.begin(), item);
	}
	else if (size_t(index) >= _items.size())
	{
		_items.push_back(item);
	}
	else 
	{
		CarouselItemList::iterator insertPos = _items.begin();
		for (int i = 0; i < index; ++i, ++insertPos);

		_items.insert(insertPos, item);
	}

	buildItems();

	return item;
}

void Carousel::buildRibbonItems()
{
	float a = 0.0f;
	float da = DOUBLE_PI / static_cast<float>(_items.size());
	float maxItems = 4.0f;
	float maxAngle = HALF_PI;
	float xOffset = _scale.x * _scale.x * maxItems / 1.5f;
	float yOffset = _scale.y * _scale.y * maxItems / 1.5f;
	float zOffset = 2.0f;

	float index = 0.0f;
	for (CarouselItem::Pointer& item : _items)
	{
		float actualIndex = (index - _selectedItem) / maxItems;
		float linear = clamp(actualIndex, -1.0f, 1.0f);
		float fvalue = etMax(0.0f, linear * linear - 0.5f);

		float angleScale = ((linear < 0.0f) ? -maxAngle : maxAngle) *
            (std::sqrt(std::abs(linear)) - std::pow(fvalue, 8.0f));

		vec3 t(_direction.x * xOffset * linear + _center.x,
			   _direction.y * yOffset * linear + _center.y, -std::abs(zOffset * linear));
		
		vec3 r(_direction.y * angleScale, _direction.x * angleScale, 0.0f);

		item->setActualIndex(actualIndex);
		item->setTransform( translationRotationYXZMatrix(t, r) );
		item->setColor(vec4(1.0f, 1.0f, 1.0f, _alpha * (1.0f - std::abs(linear))));

		a += da;
		index += 1.0f;
	}
}

void Carousel::buildRoundItems()
{
	float a = 0.0f;
	float da = DOUBLE_PI / static_cast<float>(_items.size());
	float z_distance = 0.5f;

	// TODO
	float selectedItemAlpha = 0.0f;

	for (CarouselItemList::iterator i = _items.begin(), e = _items.end(); i != e; ++i)
	{
		float alpha = 0.5f + 0.5f * std::cos(selectedItemAlpha + a);
		(*i)->setTransform(translationMatrix(0.0f, 0.0f, z_distance) * rotationYXZMatrix(0.0f, a, 0.0f));
		(*i)->setAngle(a);
		(*i)->setColor(vec4(1.0f, 1.0f, 1.0f, alpha));

		a += da;
	}
}

void Carousel::buildItems()
{
	if (_items.size())
	{
		buildRibbonItems();
		sortItems();
	}

	invalidateContent();
}

void Carousel::setSelectedItem(size_t item, float duration)
{
	if (_type == CarouselType_Ribbon)
	{
		item = clamp(item, static_cast<size_t>(item), _items.size() - 1);
		float newSelectedItem = static_cast<float>(item);

		if (duration > 0.0f)
		{
			_animating = true;
			_setItemAnimator.animate(&_selectedItem, _selectedItem, newSelectedItem, duration);
		}
		else 
		{
			_selectedItem = newSelectedItem;
			buildItems();
		}

		CarouselItemList::iterator i = _items.begin();
		std::advance(i, item);
		didSelectItem.invoke(*i);
	}
}

void Carousel::sortItems()
{
	_sortedItems = CarouselItemList(_items.begin(), _items.end());
	_sortedItems.sort(_type == CarouselType_Round ? roundDistanceFunc : ribbonDistanceFunc);
}

void Carousel::animatorUpdated(BaseAnimator*)
{
	buildItems();
}

void Carousel::animatorFinished(BaseAnimator* a)
{
	if (a == &_setItemAnimator)
		_animating = false;
}

size_t Carousel::selectedItem() const
{
	float f = floorf(_selectedItem);
	size_t result = static_cast<size_t>(f);
	return (_selectedItem - f > 0.5f) ? result + 1 : result;
} 

bool Carousel::pointerPressed(const PointerInputInfo& p)
{
	size_t item = 0;
	if ((_alpha < 1.0e-4) || (_dragOnlyItems && itemForInputInfo(p, item).invalid())) return false;

	if (p.type == PointerType_General)
	{
		_clickTime = actualTime();
		_waitingClick = true;
		_lastTouch = p;
		_animating = false;
		_pointerPressed = true;
	}
	else 
	{
		_waitingClick = false;
	}

	CarouselItem::Pointer cItem = itemForInputInfo(p, item);
	
	if (cItem.valid())
		cItem->setColor(vec4(0.5f, 0.5f, 0.5f, cItem->color().w));

	return cItem.valid();
}

bool Carousel::pointerMoved(const PointerInputInfo& p)
{
	size_t item = 0;
	if (!_dragging && _dragOnlyItems && itemForInputInfo(p, item).invalid()) return false;

	if ((p.type == PointerType_General))
	{
		if (_dragging)
		{
			float dt = p.timestamp - _lastTouch.timestamp;

			if (dt > 0.0f)
			{
				float ds = dot(_lastTouch.normalizedPos - p.normalizedPos, _direction) / dt;
				_velocity += ds;

				if (std::abs(_velocity) > maxVelocity)
					_velocity = signNoZero(_velocity) * maxVelocity;

			}

			_lastTouch = p;
		}
		else if (_pointerPressed)
		{
			vec2 dp = p.pos - _lastTouch.pos;
			if (dp.dotSelf() > 1.0f)
			{
				_waitingClick = false;
				_dragging = true;
				_lastTouch = p;
			}
		}
	}

	return itemForInputInfo(p, item).valid();
}

bool Carousel::pointerReleased(const PointerInputInfo& p)
{
	size_t item = 0;
	if (!_dragging && _dragOnlyItems && itemForInputInfo(p, item).invalid()) return false;

	bool processed = _dragging;
	if (p.type == PointerType_General)
	{
		_dragging = false;

		if (_waitingClick)
		{
			performClick(p);
			processed = true;
		}
		else if (!_animating)
		{
			alignSelectedItem(false);
		}

		_waitingClick = false;
		_pointerPressed = false;
	}

	for (CarouselItem::Pointer c : children())
		c->setColor(vec4(1.0f, 1.0f, 1.0f, c->color().w));

	return processed;
}

bool Carousel::pointerScrolled(const PointerInputInfo&)
{
	return false;
}

void Carousel::pointerEntered(const PointerInputInfo&)
{
}

void Carousel::pointerLeaved(const PointerInputInfo&)
{
}

bool Carousel::containsPoint(const vec2& p, const vec2& np)
{
	size_t iIndex = 0;
	CarouselItem::Pointer item = itemForInputInfo(PointerInputInfo(PointerType_General, p, np,
		vec2(0.0f), 0, 0.0f, PointerOrigin_Any), iIndex);
	
	return (_alpha > 0.0f) && (_dragOnlyItems ? item.valid() : true);
}

void Carousel::alignSelectedItem(bool)
{
	float fvel = std::abs(_velocity);
	if (_dragging || (fvel > minUpdateVelocity)) return;

	_velocity = 0.0f;
	float f = clamp(std::floor(_selectedItem), 0.0f, static_cast<float>(_items.size() - 1));
	size_t newItem = static_cast<size_t>(f);
	newItem += (_selectedItem - f) > 0.5f ? 1 : 0;

	setSelectedItem(newItem, defaultAnimationDuration);
}

void Carousel::update(float t)
{
	if (_waitingClick)
	{
		float dClickTime = t - _clickTime;
		if (dClickTime > clickTimeout)
		{
			_waitingClick = false;
			_dragging = true;
		}
	}

	float dt = t - _lastUpdateTime;
	_lastUpdateTime += dt;

	float fvel = std::abs(_velocity);
	if (!_animating && (fvel != 0.0f))
	{
		float dv = etMin(1.0f, dt * slowdownCoefficient);

		_selectedItem += dt * _velocity;
		_velocity -= dv * _velocity;

		if (std::abs(_velocity) < minUpdateVelocity)
			alignSelectedItem(true);
		else
			buildItems();
	}
}

bool Carousel::performClick(const PointerInputInfo& p)
{
	size_t index = 0;

	CarouselItem::Pointer item = itemForInputInfo(p, index);
	if (item.valid())
	{
		if (std::abs(_selectedItem - index) > minUpdateDelta)
		{
			setSelectedItem(index, defaultAnimationDuration);
		}
		else 
		{
			item->selected.invoke(item);
			itemClicked.invoke(item);
		}
	}

	return item.valid();
}

void Carousel::setCenter(const vec2& c, float duration)
{
	if (duration > 0.0f)
	{
		_positionAnimator.animate(&_center, _center, c, duration);
	}
	else 
	{
		_center = c;
		buildItems();
	}
}

CarouselItem::Pointer Carousel::itemForInputInfo(const PointerInputInfo& p, size_t& index)
{
	size_t aIndex = 0;

	ray3d r = camera().castRay(p.normalizedPos);
	for (CarouselItemList::iterator i = _items.begin(), e = _items.end(); i != e; ++i, ++aIndex)
	{
		CarouselItem::Pointer& item = *i;
		if (item->rayIntersect(r))
		{
			index = aIndex;
			return item;
		}
	}

	return CarouselItem::Pointer();
}

void Carousel::setAlpha(float value, float duration)
{
	if (value == 0.0f)
	{
		_dragging = false;
		_waitingClick = false;
		_pointerPressed = false;
	}

	if (duration > 0.0f)
	{
		_alphaAnimator.animate(&_alpha, _alpha, value, duration);
	}
	else
	{
		_alpha = value;
		buildItems();
	}
}

void Carousel::clear()
{
	_items.clear();
	removeChildren();
	buildItems();
}

void Carousel::setDragOnlyItems(bool value)
{
	_dragOnlyItems = value;
}

void Carousel::setDirection(const vec2& d)
{
	_direction = d;
	buildItems();
}

/*
 * Helper functions
 */
bool roundDistanceFunc(const CarouselItem::Pointer& i1, const CarouselItem::Pointer& i2)
	{ return cos(i1->currentAngle()) < cos(i2->currentAngle()); }

bool ribbonDistanceFunc(const CarouselItem::Pointer& i1, const CarouselItem::Pointer& i2)
	{ return i1->transform()[3][2] < i2->transform()[3][2]; }
