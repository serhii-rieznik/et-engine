/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/applicationnotifier.h>
#include <et/gui/layout.h>

using namespace et;
using namespace et::gui;

ET_DECLARE_GUI_ELEMENT_CLASS(Layout)

Layout::Layout() : Element2d(nullptr, ElementClass<decltype(this)>::uniqueName(std::string())),
	_currentElement(nullptr), _focusedElement(nullptr), _capturedElement(nullptr), _valid(false),
	_dragging(false)
{
	setAutolayot(vec2(0.0f), LayoutMode_Absolute, vec2(1.0f),
		LayoutMode_RelativeToContext, vec2(0.0f));
}

void Layout::layout(const vec2& sz)
{
	autoLayout(sz);
	layoutChildren();
}

void Layout::adjustVerticalOffset(float) { }
void Layout::resetVerticalOffset() { }

void Layout::addElementToRenderQueue(Element* element, RenderContext* rc, GuiRenderer& gr)
{
	if (!element->visible()) return;

	bool clipToBounds = element->hasFlag(Flag_ClipToBounds);

	if (clipToBounds)
	{
		mat4 parentTransform = element->parent()->finalTransform();
		vec2 eSize = multiplyWithoutTranslation(element->size(), parentTransform);
		vec2 eOrigin = parentTransform * element->origin();
		
		gr.pushClipRect(recti(vec2i(static_cast<int>(eOrigin.x),
			static_cast<int>(rc->size().y - eOrigin.y - eSize.y)),
			vec2i(static_cast<int>(eSize.x), static_cast<int>(eSize.y))));
	}
	
	element->addToRenderQueue(rc, gr);
	for (auto& c : element->children())
	{
		if (!elementIsBeingDragged(c.ptr()))
			addElementToRenderQueue(c.ptr(), rc, gr);
	}
	element->addToOverlayRenderQueue(rc, gr);
	
	if (clipToBounds)
		gr.popClipRect();
}

void Layout::addToRenderQueue(RenderContext* rc, GuiRenderer& gr)
{
	gr.resetClipRect();
	
	for (auto& c : children())
	{
		if (!elementIsBeingDragged(c.ptr()))
			addElementToRenderQueue(c.ptr(), rc, gr);
	}

	for (auto& t : _topmostElements)
	{
		if (!elementIsBeingDragged(t.ptr()))
			addElementToRenderQueue(t.ptr(), rc, gr);
	}
	
	if (elementIsBeingDragged(_capturedElement))
		addElementToRenderQueue(_capturedElement, rc, gr);
	
	_valid = true;
}

bool Layout::pointerPressed(const et::PointerInputInfo& p)
{ 
	if (hasFlag(Flag_TransparentForPointer)) return false;

	if (_capturedElement)
	{
		_capturedElement->pointerPressed(PointerInputInfo(p.type, _capturedElement->positionInElement(p.pos), 
			p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		return true;
	}
	else 
	{
		Element* active = activeElement(p);
		setCurrentElement(p, active);
		bool processed = false;
		if (!active)
		{
			setActiveElement(nullptr);
		}
		else
		{
			vec2 elementPos = active->positionInElement(p.pos);
			
			processed = active->pointerPressed(PointerInputInfo(p.type, elementPos,
				p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

			if ((p.type == PointerType_General))
			{
				if (active->hasFlag(Flag_Dragable))
				{
					processed = true;

					_dragging = true;
					_capturedElement = active;
					_dragInitialPosition = active->position();
					_dragInitialOffset = elementPos;
					
					_capturedElement->dragStarted.invoke(_capturedElement,
						ElementDragInfo(_dragInitialPosition, _dragInitialPosition, p.normalizedPos));

					if (Input::canGetCurrentPointerInfo())
						startUpdates();
					
					invalidateContent();
				}
				else
				{
					setActiveElement(active);
				}
			}

			if (processed || _dragging)
			{
				_capturedElement = active;
				invalidateContent();
			}
		}

		return processed;
	}
}

bool Layout::pointerMoved(const et::PointerInputInfo& p)
{
	if (hasFlag(Flag_TransparentForPointer)) return false;

	if (_capturedElement)
	{
		if (!Input::canGetCurrentPointerInfo() && (p.type == PointerType_General) && _dragging)
			performDragging(p);

		_capturedElement->pointerMoved(PointerInputInfo(p.type, _capturedElement->positionInElement(p.pos),
			p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

		return true;
	}
	else 
	{
		Element* active = activeElement(p);
		
		setCurrentElement(p, active);

		bool processed = active && active->pointerMoved(PointerInputInfo(p.type,
			active->positionInElement(p.pos), p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

		return processed;
	}
}

bool Layout::pointerReleased(const et::PointerInputInfo& p)
{ 
	if (hasFlag(Flag_TransparentForPointer)) return false;

	Element* active = activeElement(p);
	if (_capturedElement)
	{
		bool processed = _capturedElement->pointerReleased(PointerInputInfo(p.type,
			_capturedElement->positionInElement(p.pos), p.normalizedPos, p.scroll, p.id,
			p.timestamp, p.origin));

		if ((p.type == PointerType_General) && _dragging)
		{
			if (Input::canGetCurrentPointerInfo())
				cancelUpdates();

			_capturedElement->dragFinished.invoke(_capturedElement,
				ElementDragInfo(_capturedElement->parent()->positionInElement(p.pos),
				_dragInitialPosition, p.normalizedPos));

			_dragging = false;
			_capturedElement = nullptr;
			invalidateContent();
		}
		else if (processed) 
		{
			_capturedElement = nullptr;
			invalidateContent();
		}

		return true;
	}
	else 
	{
		setCurrentElement(p, active);
		bool processed = false;

		if (active)
		{
			processed = active->pointerReleased(PointerInputInfo(p.type, active->positionInElement(p.pos), 
				p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		}

		return processed;
	}
}

bool Layout::pointerScrolled(const et::PointerInputInfo& p)
{ 
	if (hasFlag(Flag_TransparentForPointer)) return false;

	if (_capturedElement)
	{
		_capturedElement->pointerScrolled(PointerInputInfo(p.type, _capturedElement->positionInElement(p.pos),
			p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		
		return true;
	}
	else 
	{
		Element* active = activeElement(p);
		setCurrentElement(p, active);

		bool processed = active && active->pointerScrolled(PointerInputInfo(p.type,
			active->positionInElement(p.pos), p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

		return processed;
	}
}

Element* Layout::activeElement(const PointerInputInfo& p)
{
	if (!_valid)
	{
		_topmostElements.clear();
		for (auto& i : children())
			collectTopmostElements(i.ptr());
	}

	Element* active = nullptr;
	for (auto i = _topmostElements.rbegin(), e = _topmostElements.rend(); i != e; ++i)
	{
		active = getActiveElement(p, i->ptr());
		if (active)	break;
	}

	if (!active)
	{
		for (auto i = children().rbegin(), e = children().rend(); i != e; ++i)
		{
			active = getActiveElement(p, i->ptr());
			if (active)	break;
		}
	}

	return active;
}

Element* Layout::getActiveElement(const PointerInputInfo& p, Element* el)
{
	if (!el->visible() || !el->enabled() || !el->containsPoint(p.pos, p.normalizedPos))
		return nullptr;
	
	if (el->hasFlag(Flag_HandlesChildEvents))
		return el;

	for (auto ei = el->children().rbegin(), ee = el->children().rend(); ei != ee; ++ei)
	{
		Element* element = getActiveElement(p, ei->ptr());
		if (element)
			return element;
	}

	return el->hasFlag(Flag_TransparentForPointer) ? nullptr : el;
}

void Layout::setCurrentElement(const PointerInputInfo& p, Element* e)
{
	if (e == _currentElement) return;

	if (_currentElement)
	{
		_currentElement->pointerLeaved(p);
		_currentElement->hoverEnded.invoke(_currentElement);
	}

	_currentElement = e;

	if (_currentElement)
	{
		_currentElement->pointerEntered(p);
		_currentElement->hoverStarted.invoke(_currentElement);
	}
}

void Layout::performDragging(const PointerInputInfo& p)
{
	assert(_dragging);
	
	vec2 currentPos = _capturedElement->positionInElement(p.pos);
	vec2 delta = currentPos - _dragInitialOffset;
	
	_capturedElement->setPosition(_capturedElement->position() + delta);
	
	_capturedElement->dragged.invoke(_capturedElement,
		ElementDragInfo(_capturedElement->position(), _dragInitialPosition, p.normalizedPos));
}

bool Layout::elementIsBeingDragged(gui::Element* e)
{
	return _dragging && (e != nullptr) && (e == _capturedElement);
}

void Layout::update(float)
{
	if (_dragging && Input::canGetCurrentPointerInfo())
		performDragging(Input::currentPointer());
}

void Layout::cancelDragging(float returnDuration)
{
	if (_dragging)
	{
		_capturedElement->setPosition(_dragInitialPosition, returnDuration);
		_capturedElement = nullptr;
		_dragging = false;
		
		invalidateContent();
	}
}

void Layout::setActiveElement(Element* e)
{
	if (_focusedElement == e) return;
	
	if (_focusedElement)
		_focusedElement->resignFocus(e);

	_focusedElement = e;

	bool needKeyboard = _focusedElement && _focusedElement->hasFlag(Flag_RequiresKeyboard);

	if (_focusedElement)
		_focusedElement->setFocus();
	
	if (needKeyboard)
		layoutRequiresKeyboard.invoke(this, _focusedElement);
	else
		layoutDoesntNeedKeyboard.invoke(this);
}

void Layout::setInvalid()
{
	_valid = false;
}

void Layout::collectTopmostElements(Element* element)
{
	if (element->visible()) return;
	
	if (element->hasFlag(Flag_RenderTopmost))
		_topmostElements.push_back(Element::Pointer(element));

	for (auto& i : element->children())
		collectTopmostElements(i.ptr());
}

void Layout::initRenderingElement(et::RenderContext* rc)
{
	if (_renderingElement.invalid())
		_renderingElement = RenderingElement::Pointer(new RenderingElement(rc));
}

vec2 Layout::contentSize()
{
	return ApplicationNotifier().accessRenderContext()->size();
}

/*
 *
 * Modal Layout
 *
 */

ModalLayout::ModalLayout()
{
	_backgroundFade = ImageView::Pointer(new ImageView(Texture(), this));
	_backgroundFade->setBackgroundColor(vec4(0.0f, 0.0f, 0.0f, 0.25f));
	_backgroundFade->fillParent();
}

bool ModalLayout::pointerPressed(const et::PointerInputInfo& p)
{
	Layout::pointerPressed(p);
	return true;
}

bool ModalLayout::pointerMoved(const et::PointerInputInfo& p)
{
	Layout::pointerMoved(p);
	return true;
}

bool ModalLayout::pointerReleased(const et::PointerInputInfo& p)
{
	Layout::pointerReleased(p);
	return true;
}

bool ModalLayout::pointerScrolled(const et::PointerInputInfo& p )
{
	Layout::pointerScrolled(p);
	return true;
}
