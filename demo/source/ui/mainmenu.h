//
//  MainMenu.h
//  ios
//
//  Created by Sergey Reznik on 07.02.13.
//  Copyright (c) 2013 Sergey Reznik. All rights reserved.
//

#pragma once

#include "resourcemanager.h"

namespace demo
{
	class MainMenuLayout : public et::gui::Layout
	{
	public:
		typedef et::IntrusivePtr<MainMenuLayout> Pointer;

	public:
		MainMenuLayout(et::RenderContext* rc, ResourceManager& resourceManager);

	private:
		et::gui::Label::Pointer _title;
	};
}
