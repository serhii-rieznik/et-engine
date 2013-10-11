/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/gui/listbox.h>
#include <et/gui/layout.h>
#include <et/gui/guirenderer.h>

using namespace et;
using namespace et::gui;

static const float popupAppearTime = 0.1f;
static const float textRevealDuration = 0.1f;

static const size_t invalidSelectedIndex = static_cast<size_t>(-1);

ET_DECLARE_GUI_ELEMENT_CLASS(ListboxPopup)

ListboxPopup::ListboxPopup(Listbox* owner, const std::string& name) :
	Element2d(owner, ET_GUI_PASS_NAME_TO_BASE_CLASS), _owner(owner), _textAlphaAnimator(0),
	_selectedIndex(invalidSelectedIndex), _textAlpha(0.0f), _pressed(false)
{
	setFlag(Flag_RenderTopmost);
}

void ListboxPopup::buildVertices(GuiRenderer& gr)
{
	mat4 transform = finalTransform();
	_backgroundVertices.setOffset(0);
	_textVertices.setOffset(0);
	_selectionVertices.setOffset(0);

	const Image& background = _owner->_background;
	const Image& selection = _owner->_selection;

	if (background.texture.valid())
	{
		gr.createImageVertices(_backgroundVertices, background.texture, background.descriptor, 
			rect(vec2(0.0), size()), color(), transform, RenderLayer_Layer0);
	}

	const StringList& values = _owner->_values;
	if (values.size() && (_textAlpha > 0.0f))
	{
		bool selectionValid = selection.texture.valid();

		float row = 0.0f;
		float rowSize = _owner->size().y;
		float y0 = floorf(0.5f * (rowSize - _owner->_font->lineHeight()));
		float dy = floorf(size().y / static_cast<float>(values.size()));

		vec4 drawColor = color();
		drawColor.w *= _textAlpha;

		vec2 textPos = _owner->_contentOffset + vec2(0.0f, y0);
		
		size_t index = 0;
		for (auto i = values.begin(), e = values.end(); i != e; ++i, ++index, row += 1.0f)
		{
			if (selectionValid && (_selectedIndex == index))
			{
				gr.createImageVertices(_selectionVertices, selection.texture, selection.descriptor, 
					rect(vec2(0.0f, row * rowSize), vec2(size().x, rowSize)), drawColor, transform,
					RenderLayer_Layer0);
			}

			gr.createStringVertices(_textVertices, _owner->_font->buildString(*i), Alignment_Near,
				Alignment_Near, textPos, drawColor, transform, RenderLayer_Layer1);
			textPos.y += dy;
		}
	}

	setContentValid();
}

void ListboxPopup::revealText()
{
	hideText();

	if (_textAlphaAnimator)
		_textAlphaAnimator->destroy();

	_textAlphaAnimator = new FloatAnimator(this, &_textAlpha, _textAlpha, 1.0f,
		textRevealDuration, 0, timerPool());
}

void ListboxPopup::hideText()
{
	_textAlpha = 0.0f;
	invalidateContent();
}

void ListboxPopup::animatorUpdated(BaseAnimator* a)
{
	if (a == _textAlphaAnimator)
		invalidateContent();

	Element2d::animatorUpdated(a);
}

void ListboxPopup::animatorFinished(BaseAnimator* a)
{
	if (a == _textAlphaAnimator)
	{
		a->destroy();
		_textAlphaAnimator = 0;
		_owner->popupDidOpen();
	}
	else
		Element2d::animatorFinished(a);
}

void ListboxPopup::addToRenderQueue(RenderContext*, GuiRenderer& gr)
{
	if (!contentValid() || !transformValid())
		buildVertices(gr);

	if (_backgroundVertices.lastElementIndex() > 0)
		gr.addVertices(_backgroundVertices, _owner->_background.texture, ElementRepresentation_2d, RenderLayer_Layer0);

	if (_selectionVertices.lastElementIndex() > 0)
		gr.addVertices(_selectionVertices, _owner->_selection.texture, ElementRepresentation_2d, RenderLayer_Layer0);

	if (_textVertices.lastElementIndex() > 0)
		gr.addVertices(_textVertices, _owner->_font->texture(), ElementRepresentation_2d, RenderLayer_Layer1);
}

bool ListboxPopup::pointerPressed(const PointerInputInfo&)
{
	_pressed = true;
	return true;
}

bool ListboxPopup::pointerMoved(const PointerInputInfo& p)
{
	_selectedIndex = static_cast<size_t>(p.pos.y / _owner->size().y);
	invalidateContent();
	return true;
}

bool ListboxPopup::pointerReleased(const PointerInputInfo& p)
{
	if (_pressed)
	{
		_selectedIndex = static_cast<size_t>(p.pos.y / _owner->size().y);
		_owner->setSelectedIndex(_selectedIndex);
		_owner->hidePopup();
	}
	_pressed = false;

	return true;
}

void ListboxPopup::pointerEntered(const PointerInputInfo&)
{

}

void ListboxPopup::pointerLeaved(const PointerInputInfo&)
{
	_selectedIndex = invalidSelectedIndex;
	invalidateContent();
}

/*
 * Listbox
 */

ET_DECLARE_GUI_ELEMENT_CLASS(Listbox)

Listbox::Listbox(Font font, Element2d* parent, const std::string& name) :
	Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _font(font), _state(ListboxState_Default),
	_contentOffset(0.0f), _selectedIndex(invalidSelectedIndex), _direction(ListboxPopupDirection_Bottom),
	_popupOpened(false), _popupOpening(false), _popupValid(false)
{
	_popup = ListboxPopup::Pointer(new ListboxPopup(this));
	_popup->setVisible(false);
	
	ET_CONNECT_EVENT(_popup->elementAnimationFinished, Listbox::onPopupAnimationFinished)
}

void Listbox::setImage(const Image& img, ListboxState state)
{
	if (state == ListboxState_Default)
	{
		_contentOffset.x = floorf(2.0f / 3.0f * img.descriptor.contentOffset.left);
		_contentOffset.y = 0;
	}

	_images[state] = img;
	invalidateContent();
}

void Listbox::setBackgroundImage(const Image& img)
{
	_background = img;
	invalidateContent();
}

void Listbox::setSelectionImage(const Image& img)
{
	_selection = img;
	invalidateContent();
}

void Listbox::buildVertices(GuiRenderer& gr)
{
	mat4 transform = finalTransform();
	_backgroundVertices.setOffset(0);
	_textVertices.setOffset(0);

	if (_images[_state].texture.valid())
	{
		gr.createImageVertices(_backgroundVertices, _images[_state].texture, _images[_state].descriptor, 
			rect(vec2(0.0), size()), color(), transform, RenderLayer_Layer0);
	}

	if (shouldDrawText())
	{
		std::string textToDraw = _prefix + _values[_selectedIndex];
		vec2 textPos = _contentOffset + vec2(0.0f, 0.5f * (size().y - _font->lineHeight()));
		gr.createStringVertices(_textVertices, _font->buildString(textToDraw), Alignment_Near, Alignment_Near,
								textPos, color(), transform, RenderLayer_Layer1);
	}

	setContentValid();
}

void Listbox::addToRenderQueue(RenderContext*, GuiRenderer& gr)
{
	if (!contentValid())
		buildVertices(gr);

	if (_images[_state].texture.valid())
		gr.addVertices(_backgroundVertices, _images[_state].texture, ElementRepresentation_2d, RenderLayer_Layer0);

	if (shouldDrawText())
		gr.addVertices(_textVertices, _font->texture(),  ElementRepresentation_2d, RenderLayer_Layer1);
}

bool Listbox::shouldDrawText()
{
	return !(_popupOpened || (_selectedIndex == invalidSelectedIndex));
}

bool Listbox::containsPoint(const vec2& p, const vec2& np)
{
	bool inPopup = _popupOpened && _popup->containsPoint(p, np);
	return Element2d::containsPoint(p, np) || inPopup;
}

void Listbox::setFrame(const rect& r, float duration)
{
	Element2d::setFrame(r, duration);
	configurePopup();
}

bool Listbox::pointerPressed(const PointerInputInfo& p)
{
	if (_popupOpened || _popupOpening)
	{
		return _popup->pointerPressed(p);
	}
	else 
	{
		if (!_popupOpening)
			showPopup();

		return true;
	}
}

bool Listbox::pointerMoved(const PointerInputInfo& p)
{
	return _popupOpened ? _popup->pointerMoved(p) : true;
}

bool Listbox::pointerReleased(const PointerInputInfo& p)
{
	return _popupOpened ? _popup->pointerReleased(p) : true;
}

void Listbox::pointerEntered(const PointerInputInfo& p)
{
	_mouseIn = true;

	if (_popupOpened || _popupOpening)
		_popup->pointerEntered(p);
	else
		setState(ListboxState_Highlighted);
}

void Listbox::pointerLeaved(const PointerInputInfo& p)
{
	_mouseIn = false;
	if (_popupOpened || _popupOpening)
		_popup->pointerLeaved(p);
	else
		setState(ListboxState_Default);
}

void Listbox::showPopup()
{
	setState(ListboxState_Opened);
	if (_popupOpening) return;

	if (!_popupValid)
		configurePopup();

	rect destSize = _popup->frame();
	rect currentSize = destSize;

	destSize.top = 0.0f;
	if (_direction == ListboxPopupDirection_Center)
		destSize.top = floorf(0.5f * size().y * (1.0f - static_cast<float>(_values.size())));
	else if (_direction == ListboxPopupDirection_Top)
		destSize.top = floorf(-size().y * static_cast<float>(_values.size() / 2.0f));

	currentSize.top = 0.0f;
	currentSize.height = size().y;

	_popupOpened = false;
	_popupOpening = true;

	_popup->hideText();
	_popup->setAlpha(0.0f);
	_popup->setFrame(currentSize);
	_popup->setFrame(destSize, popupAppearTime);
	_popup->setAlpha(1.0f, popupAppearTime);
	_popup->setVisible(true);

	owner()->setActiveElement(this);
}

void Listbox::hidePopup()
{
	if (_popupOpening) return;

	setState(_mouseIn ? ListboxState_Highlighted : ListboxState_Default);
	_popupOpened = false;
	_popup->setVisible(false);
	popupClosed.invoke(this);
}

void Listbox::resignFocus(Element* e)
{
	if (e != _popup.ptr())
		hidePopup();
}

void Listbox::setValues(const StringList& v)
{
	_values = v;
	_popupValid = false;
}

void Listbox::addValue(const std::string& v)
{
	_values.push_back(v);
	_popupValid = false;
}

void Listbox::configurePopup()
{
	vec2 ownSize = size();
	size_t numElements = _values.size();
	float scale = (numElements < 1) ? 1.0f : static_cast<float>(numElements);

	_popup->setFrame(0.0f, 0.0f, ownSize.x, ownSize.y * scale);
	_popupValid = true;
}

void Listbox::setState(ListboxState s)
{
	if (_state != s)
	{
		_state = s;
		invalidateContent();
	}
}

void Listbox::setSelectedIndex(size_t value)
{
	_selectedIndex = (value < _values.size()) ? value : invalidSelectedIndex;
	invalidateContent();
}

void Listbox::onPopupAnimationFinished(Element2d*, AnimatedPropery)
{
	_popup->revealText();
	popupOpened.invoke(this);
}

void Listbox::popupDidOpen()
{
	_popupOpening = false;
	_popupOpened = true;
}

void Listbox::setPrefix(const std::string& prefix)
{
	_prefix = prefix;
	invalidateContent();
}

void Listbox::setPopupDirection(ListboxPopupDirection d)
{
	_direction = d;
}