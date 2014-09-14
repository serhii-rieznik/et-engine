/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/gui/guibase.h>

using namespace et;
using namespace et::gui;

float et::gui::alignmentFactor(Alignment a)
{
	static float alignmentValues[Alignment_max] = { 0.0f, 0.5f, 1.0f };
	return alignmentValues[a];
}

State et::gui::adjustState(State s)
{
	if (!Input::canGetCurrentPointerInfo() && ((s == State_Hovered) || (s == State_SelectedHovered)))
		return static_cast<State>(s - 1);
	
	return s;
}

Element::Element(Element* parent, const std::string& name) : ElementHierarchy(parent), tag(0), _name(name),
	_enabled(true), _transformValid(false), _inverseTransformValid(false), _contentValid(false)
{
	
}

void Element::setParent(Element* element)
{
	ElementHierarchy::setParent(element);
	invalidateContent();
	invalidateTransform();
}

bool Element::enabled() const
{
	return _enabled;
}

void Element::setEnabled(bool enabled)
{
	_enabled = enabled;
}

void Element::invalidateContent()
{ 
	_contentValid = false; 

	for (Element::List::iterator i = children().begin(), e = children().end(); i != e; ++i)
		(*i)->invalidateContent();

	setInvalid();
}

void Element::invalidateTransform()
{ 
	_transformValid = false; 
	_inverseTransformValid = false;

	for (Element::List::iterator i = children().begin(), e = children().end(); i != e; ++i)
		(*i)->invalidateTransform();

	setInvalid();
}

void Element::addToRenderQueue(RenderContext*, GuiRenderer&) 
{
}

void Element::addToOverlayRenderQueue(RenderContext*, GuiRenderer&)
{
}

void Element::startUpdates(TimerPool* timerPool)
{
	TimedObject::startUpdates(timerPool);
}

void Element::startUpdates()
{
	TimedObject::startUpdates();
}

TimerPool* Element::timerPool()
{
	return mainTimerPool().ptr();
}

void Element::layoutChildren()
{
	for (Element::List::iterator i = children().begin(), e = children().end(); i != e; ++i)
		(*i)->layout(size());
}

void Element::bringToFront(Element* c)
{
	ElementHierarchy::bringToFront(c);
	invalidateContent();
}

void Element::sendToBack(Element* c)
{
	ElementHierarchy::sendToBack(c);
	invalidateContent();
}

void Element::broardcastMessage(const GuiMessage& msg)
{
	for (auto& c : children())
	{
		c->processMessage(msg);
		c->broardcastMessage(msg);
	}
}

Element* Element::baseChildWithName(const std::string& name)
	{ return childWithNameCallback(name, this); }

Element* Element::childWithNameCallback(const std::string& name, Element* root)
{
	if (root->name() == name)
		return root;

	for (auto& c : root->children())
	{
		Element* aElement = childWithNameCallback(name, c.ptr());
		if (aElement != nullptr)
			return aElement;
	}

	return nullptr;
}



void Element::setAutolayot(const et::gui::ElementLayout& al)
{
	_autoLayout = al;
}

void Element::setAutolayot(const vec2& pos, LayoutMode pMode, const vec2& sz,
							 LayoutMode sMode, const vec2& pivot)
{
	_autoLayout.position = pos;
	_autoLayout.size = sz;
	_autoLayout.pivotPoint = pivot;
	_autoLayout.positionMode = pMode;
	_autoLayout.sizeMode = sMode;
}

void Element::setAutolayoutMask(size_t m)
{
	_autoLayout.mask = m;
}

void Element::autoLayout(const vec2& contextSize, float duration)
{
	if ((_autoLayout.mask & LayoutMask_Pivot) == LayoutMask_Pivot)
		setPivotPoint(_autoLayout.pivotPoint);

	vec2 aSize = size();
	vec2 aPos = position();

	if ((_autoLayout.mask & LayoutMask_Size) == LayoutMask_Size)
	{
		if (_autoLayout.sizeMode == LayoutMode_RelativeToContext)
			aSize = contextSize * _autoLayout.size;
		else if ((_autoLayout.sizeMode == LayoutMode_RelativeToParent) && (parent() != nullptr))
			aSize = parent()->size() * _autoLayout.size;
		else if (_autoLayout.sizeMode == LayoutMode_WrapContent)
			aSize = contentSize();
		else
			aSize = _autoLayout.size;
	}

	if ((_autoLayout.mask & LayoutMask_Position) == LayoutMask_Position)
	{
		if (_autoLayout.positionMode == LayoutMode_RelativeToContext)
			aPos = contextSize * _autoLayout.position;
		else if ((_autoLayout.positionMode == LayoutMode_RelativeToParent) && (parent() != nullptr))
			aPos = parent()->size() * _autoLayout.position;
		else if (_autoLayout.positionMode == LayoutMode_WrapContent)
			abort();
		else
			aPos = _autoLayout.position;
	}

	if ((_autoLayout.mask & LayoutMask_Frame) != 0)
		setFrame(aPos, aSize, duration);

	if (!hasFlag(Flag_HandlesChildLayout))
	{
		for (auto aChild : children())
			aChild->autoLayout(contextSize, duration);
	}
}

void Element::fillParent()
{
	setAutolayot(vec2(0.0f), LayoutMode_RelativeToParent, vec2(1.0f),
		LayoutMode_RelativeToParent, vec2(0.0f));
}

void Element::centerInParent()
{
	setAutolayot(vec2(0.5f), LayoutMode_RelativeToParent, vec2(1.0f),
		LayoutMode_RelativeToParent, vec2(0.5f));
	setAutolayoutMask(LayoutMask_PositionPivot);
}

void Element::setAutolayotRelativeToParent(const vec2& pos, const vec2& sz, const vec2& pivot)
{
	setAutolayot(pos, LayoutMode_RelativeToParent, sz, LayoutMode_RelativeToParent, pivot);
}

void Element::setAutolayoutSizeMode(LayoutMode mode)
{
	_autoLayout.sizeMode = mode;
}
