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
	void excludeFileFromICloudBackup(const std::string&);
	
	void saveImageToPhotos(const std::string&, void* context, void(*callback)(bool, void*));
	
	void shareFile(const std::string& path, const std::string& scheme, bool displayOptions);
	
	bool canOpenURL(const std::string&);
}
