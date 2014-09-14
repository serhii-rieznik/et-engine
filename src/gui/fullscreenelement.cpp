/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/gui/fullscreenelement.h>

using namespace et;
using namespace et::gui;

ET_DECLARE_GUI_ELEMENT_CLASS(FullscreenElement)

FullscreenElement::FullscreenElement(Element* parent, const std::string& name) :
	Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS)
{
	setFlag(Flag_TransparentForPointer);
}

void FullscreenElement::layout(const vec2& sz)
{
	setFrame(vec2(0.0f), sz);
	autoLayout(sz);
	layoutChildren();
}
