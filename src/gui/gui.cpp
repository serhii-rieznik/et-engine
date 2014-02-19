/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/gui/gui.h>

using namespace et;
using namespace et::gui;

#if (ET_PLATFORM_IOS || ET_PLATFORM_ANDROID)
static const bool shouldSaveFillRate = true;
#else
static const bool shouldSaveFillRate = false;
#endif

Gui::Gui(RenderContext* rc) : _rc(rc),  _renderer(rc, shouldSaveFillRate),
	_renderingElementBackground(new RenderingElement(rc)),
	_background(Texture(), 0), _backgroundValid(true)
{
	_background.setPivotPoint(vec2(0.5f));
	_background.setContentMode(ImageView::ContentMode_Fill);
	layout(rc->size());
}

bool Gui::pointerPressed(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;

	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerPressed(p))
			return true;
	}
	return false;
}

bool Gui::pointerMoved(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;

	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerMoved(p))
			return true;
	}

	return false;
}

bool Gui::pointerReleased(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;

	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerReleased(p))
			return true;
	}

	return false;
}

bool Gui::pointerCancelled(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;
	
	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerCancelled(p))
			return true;
	}
	
	return false;
}

bool Gui::pointerScrolled(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;

	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerScrolled(p))
			return true;
	}

	return false;
}

bool Gui::characterEntered(size_t p)
{
	if (_keyboardFocusedLayout.invalid() && _keyboardFocusedElement.invalid()) return false;
	
	_keyboardFocusedElement->processMessage(GuiMessage(GuiMessage::Type_TextInput, p));
	return true;
}

void Gui::buildLayoutVertices(RenderContext* rc, RenderingElement::Pointer element, Layout::Pointer layout)
{
	_renderer.setRendernigElement(element);

	if (!layout->valid())
	{
		element->clear();
		layout->addToRenderQueue(rc, _renderer);
	}
}

void Gui::buildBackgroundVertices(RenderContext* rc)
{
	if (!_backgroundValid)
	{
		_renderingElementBackground->clear();
		_background.addToRenderQueue(rc, _renderer);
	}
	_backgroundValid = true;
}

void Gui::layout(const vec2& size)
{
	_screenSize = size;
	_renderer.setProjectionMatrices(size);
	_background.setFrame(0.5f * size, size);
	_backgroundValid = false;

	for (auto& i : _layouts)
		i->layout->layout(size);
}

void Gui::render(RenderContext* rc)
{
	_renderer.beginRender(rc);

	if (_background.texture().valid())
	{
		_renderer.setRendernigElement(_renderingElementBackground);
		buildBackgroundVertices(rc);
		_renderer.setCustomAlpha(1.0f);
		_renderer.setCustomOffset(vec2(0.0f));
		_renderer.render(rc);
	}

	for (auto& obj : _layouts)
	{
		buildLayoutVertices(rc, obj->layout->renderingElement(), obj->layout);
		_renderer.setCustomAlpha(obj->offsetAlpha.z);
		_renderer.setCustomOffset(obj->offsetAlpha.xy());
		_renderer.render(rc);
	}

	_renderer.endRender(rc);
}

void Gui::setBackgroundImage(const Image& img)
{
	_background.setImage(img);
	_backgroundValid = false;
}

void Gui::onKeyboardNeeded(Layout* l, Element* element)
{
	_keyboardFocusedElement = Element::Pointer(element);
	_keyboardFocusedLayout = Layout::Pointer(l);
	input().activateSoftwareKeyboard();
}

void Gui::onKeyboardResigned(Layout*)
{
	input().deactivateSoftwareKeyboard();
	_keyboardFocusedElement.reset(0);
	_keyboardFocusedLayout.reset(0);
}

void Gui::getAnimationParams(size_t flags, vec3* nextSrc, vec3* nextDst, vec3* currDst)
{
	float fromLeft = static_cast<float>((flags & AnimationFlag_FromLeft) == AnimationFlag_FromLeft);
	float fromRight = static_cast<float>((flags & AnimationFlag_FromRight) == AnimationFlag_FromRight);
	float fromTop = static_cast<float>((flags & AnimationFlag_FromTop) == AnimationFlag_FromTop);
	float fromBottom = static_cast<float>((flags & AnimationFlag_FromBottom) == AnimationFlag_FromBottom);
	float fade = static_cast<float>((flags & AnimationFlag_Fade) == AnimationFlag_Fade);

	if (nextSrc)
		*nextSrc = vec3(-1.0f * fromLeft + 1.0f * fromRight, 1.0f * fromTop - 1.0f * fromBottom, 1.0f - fade);

	if (nextDst)
		*nextDst = vec3(0.0, 0.0, 1.0f);

	if (currDst)
		*currDst = vec3(1.0f * fromLeft - 1.0f * fromRight, -1.0f * fromTop + 1.0f * fromBottom, 1.0f - fade);
}

void Gui::internal_replaceTopmostLayout(Layout::Pointer newLayout, AnimationDescriptor desc)
{
	removeLayout(topmostLayout(), desc.flags, desc.duration);
	pushLayout(newLayout, desc.flags, desc.duration);
}

void Gui::internal_replaceLayout(LayoutPair l, AnimationDescriptor desc)
{
	auto i = _layouts.begin();
	while (i != _layouts.end())
	{
		if ((*i)->layout == l.oldLayout)
			break;

		++i;
	}

	removeLayout(l.oldLayout, desc.flags, desc.duration);

	if (i == _layouts.end())
	{
		pushLayout(l.newLayout, desc.flags, desc.duration);
	}
	else 
	{
		LayoutEntry newEntry(new LayoutEntryObject(this, _rc, l.newLayout));
		_layouts.insert(i, newEntry);

		animateLayoutAppearing(l.newLayout, newEntry.ptr(), desc.flags, desc.duration);
	}
}

void Gui::internal_removeLayout(Layout::Pointer oldLayout, AnimationDescriptor desc)
{
	LayoutEntryObject* entry = entryForLayout(oldLayout);
	if (entry == 0) return;

	layoutWillDisappear.invoke(oldLayout);
	oldLayout->willDisappear();
	oldLayout->layoutDoesntNeedKeyboard.disconnect(this);
	oldLayout->layoutRequiresKeyboard.disconnect(this);

	if ((desc.flags == AnimationFlag_None) || (std::abs(desc.duration) < std::numeric_limits<float>::epsilon()))
	{
		oldLayout->didDisappear();
		removeLayoutFromList(oldLayout);
	}
	else 
	{
		vec3 destOffsetAlpha;
		getAnimationParams(desc.flags, 0, 0, &destOffsetAlpha);
		entry->animateTo(destOffsetAlpha, std::abs(desc.duration), Gui::LayoutEntryObject::State_Disappear);
	}
}

void Gui::internal_pushLayout(Layout::Pointer newLayout, AnimationDescriptor desc)
{
	if (newLayout.invalid()) return;

	if (hasLayout(newLayout))
		internal_removeLayout(newLayout, AnimationDescriptor());

	_layouts.push_back(LayoutEntry(new LayoutEntryObject(this, _rc, newLayout)));
	animateLayoutAppearing(newLayout, _layouts.back().ptr(), desc.flags, desc.duration);
}

void Gui::animateLayoutAppearing(Layout::Pointer newLayout, LayoutEntryObject* newEntry,
	size_t animationFlags, float duration)
{
	newLayout->layout(_screenSize);

	layoutWillAppear.invoke(newLayout);
	newLayout->willAppear();

	ET_CONNECT_EVENT(newLayout->layoutRequiresKeyboard, Gui::onKeyboardNeeded)
	ET_CONNECT_EVENT(newLayout->layoutDoesntNeedKeyboard, Gui::onKeyboardResigned)

	bool smallDuration = std::abs(duration) < std::numeric_limits<float>::epsilon();
	if ((animationFlags == AnimationFlag_None) || smallDuration)
	{
		newLayout->didAppear();
		layoutDidAppear.invoke(newLayout);
	}
	else 
	{
		vec3 destOffsetAlpha;
		getAnimationParams(animationFlags, &newEntry->offsetAlpha, &destOffsetAlpha, 0);
		newEntry->animateTo(destOffsetAlpha, std::abs(duration), Gui::LayoutEntryObject::State_Appear);
	}
}

void Gui::removeLayoutFromList(Layout::Pointer ptr)
{
	for (auto i = _layouts.begin(), e = _layouts.end(); i != e; ++i)
	{
		if ((*i)->layout == ptr)
		{
			_layouts.erase(i);
			return;
		}
	}
}

void Gui::removeLayoutEntryFromList(LayoutEntryObject* ptr)
{
	for (auto i = _layouts.begin(), e = _layouts.end(); i != e; ++i)
	{
		if (i->ptr() == ptr)
		{
			_layouts.erase(i);
			return;
		}
	}
}

void Gui::layoutEntryTransitionFinished(LayoutEntryObject* l)
{
	if (l->state == LayoutEntryObject::State_Disappear)
	{
		layoutDidDisappear.invoke(l->layout);
		l->layout->didDisappear();

		removeLayoutEntryFromList(l);
	}
	else 
	{
		layoutDidAppear.invoke(l->layout);
		l->layout->didAppear();

		l->state = Gui::LayoutEntryObject::State_Still;
	}
}

bool Gui::hasLayout(Layout::Pointer aLayout)
{
	if (aLayout.invalid()) return false;

	for (auto& i : _layouts)
	{
		if (i->layout == aLayout)
			return true;
	}

	return false;
}

Gui::LayoutEntryObject* Gui::entryForLayout(Layout::Pointer ptr)
{
	if (ptr.invalid()) return nullptr;

	for (auto& i : _layouts)
	{
		if (i->layout == ptr)
			return i.ptr();
	}

	return nullptr;
}

bool Gui::animatingTransition()
{
	for (auto& i : _layouts)
	{
		if (i.ptr()->state != Gui::LayoutEntryObject::State_Still)
			return true;
	}
	
	return false;
}

/*
 * Layout Entry
 */

Gui::LayoutEntryObject::LayoutEntryObject(Gui* own, RenderContext* rc, Layout::Pointer l) :
	animator(l->timerPool()), layout(l), offsetAlpha(0.0f, 0.0f, 1.0f), state(Gui::LayoutEntryObject::State_Still)
{
	l->initRenderingElement(rc);
	animator.finished.connect([this, own](){ own->layoutEntryTransitionFinished(this); });
}

void Gui::LayoutEntryObject::animateTo(const vec3& oa, float duration, State s)
{
	state = s;
	animator.animate(&offsetAlpha, offsetAlpha, oa, duration);
}

void Gui::showMessageView(MessageView::Pointer mv, size_t animationFlags, float duration)
{
	ET_CONNECT_EVENT(mv->messageViewButtonSelected, Gui::onMessageViewButtonClicked)
	pushLayout(mv, animationFlags, duration);
}

void Gui::onMessageViewButtonClicked(MessageView* view, MessageViewButton)
{
	removeLayout(Layout::Pointer(view));
}

void Gui::replaceTopmostLayout(Layout::Pointer newLayout, size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Gui, internal_replaceTopmostLayout, newLayout,
		AnimationDescriptor(animationFlags, duration))
}

void Gui::popTopmostLayout(size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Gui, internal_removeLayout, topmostLayout(),
		AnimationDescriptor(animationFlags, duration))
}

void Gui::replaceLayout(Layout::Pointer oldLayout, Layout::Pointer newLayout,
	size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Gui, internal_replaceLayout, LayoutPair(oldLayout, newLayout),
		AnimationDescriptor(animationFlags, duration))
}

void Gui::removeLayout(Layout::Pointer oldLayout, size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Gui, internal_removeLayout, oldLayout,
		AnimationDescriptor(animationFlags, duration))
}

void Gui::pushLayout(Layout::Pointer newLayout, size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Gui, internal_pushLayout, newLayout,
		AnimationDescriptor(animationFlags, duration))
}

