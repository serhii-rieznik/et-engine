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
	namespace ios
	{
		void saveImageToPhotos(const std::string& path, std::function<void(bool)> callback);
		
		void excludeFileFromICloudBackup(const std::string&);
		void shareFile(const std::string& path, const std::string& scheme, bool displayOptions);
		bool canOpenURL(const std::string&);
		
		std::string hardwareIdentifier();
	}
}
