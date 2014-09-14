/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/element2d.h>

namespace et
{
	namespace gui
	{
		class FullscreenElement : public Element2d
		{
		public:
			typedef IntrusivePtr<FullscreenElement> Pointer;

		public:
			FullscreenElement(Element* parent, const std::string& name = std::string());
			void layout(const vec2& sz);
		};
	}
}
