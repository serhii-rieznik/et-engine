/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	struct Screen
	{
		std::string deviceId;
		recti frame;
		recti availableFrame;
		int scaleFactor = 1;
		
		Screen()
			{ }
		
		Screen(const recti& f, const recti& a, int sf) :
			frame(f), availableFrame(a), scaleFactor(sf) { }
	};
	
	Screen currentScreen();
	std::vector<Screen> availableScreens();
}
