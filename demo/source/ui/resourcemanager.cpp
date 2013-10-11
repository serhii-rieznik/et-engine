//
//  resourcemanager.cpp
//  ios
//
//  Created by Sergey Reznik on 07.02.13.
//  Copyright (c) 2013 Sergey Reznik. All rights reserved.
//

#include "resourcemanager.h"

using namespace et;
using namespace demo;

ResourceManager::ResourceManager()
{
}

void ResourceManager::load(et::RenderContext* rc)
{
	gui::CharacterGenerator::Pointer mainChars(
		new gui::CharacterGenerator(rc, "Arial", "Arial", 14 * rc->screenScaleFactor()));

	fonts.main = gui::Font(mainChars);
}

et::gui::Label::Pointer ResourceManager::label(const std::string& title, et::gui::Element2d* parent)
{
	et::gui::Label::Pointer result(new gui::Label(title, fonts.main, parent));
	return result;
}