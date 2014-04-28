/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <et/platform-ios/iap.h>
#include <et/platform-ios/imagepicker.h>
#include <et/platform-ios/mailcomposer.h>

namespace et
{
	namespace mac
	{
		bool canOpenURL(const std::string&);
		std::string bundleVersion();
	}
}
