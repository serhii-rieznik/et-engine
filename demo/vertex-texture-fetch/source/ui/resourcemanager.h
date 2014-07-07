//
//  resourcemanager.h
//  ios
//
//  Created by Sergey Reznik on 07.02.13.
//  Copyright (c) 2013 Sergey Reznik. All rights reserved.
//

#pragma once 

#include <et/gui/gui.h>

namespace demo
{
	class ResourceManager
	{
	public:
		ResourceManager();

		void load(et::RenderContext*);

		et::gui::Label::Pointer label(const std::string& title, et::gui::Element2d* parent);

	public:
		struct
		{
			et::gui::Font main;
		} fonts;

	private:
		ET_DENY_COPY(ResourceManager)
	};
}
