/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#if (ET_PLATFORM_IOS)

#include <UIKit/UIImage.h>
#include <UIKit/UIPrintInteractionController.h>
#include <et/platform-ios/printer.h>

using namespace et;

void et::printer::printImageFromFile(const std::string& s)
{
	UIImage* image = [UIImage imageWithContentsOfFile:[NSString stringWithUTF8String:s.c_str()]];
	[[UIPrintInteractionController sharedPrintController] setPrintingItem:image];
	[[UIPrintInteractionController sharedPrintController] presentAnimated:YES completionHandler:nil];
}

#endif // ET_PLATFORM_IOS
