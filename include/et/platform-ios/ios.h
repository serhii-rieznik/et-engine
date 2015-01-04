/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/platform-ios/imagepicker.h>
#include <et/platform-ios/mailcomposer.h>

namespace et
{
	namespace ios
	{
		void setIdleTimerEnabled(bool);
		
		void saveImageToPhotos(const std::string& path, std::function<void(bool)> callback);
		
		void excludeFileFromICloudBackup(const std::string&);
		void shareFile(const std::string& path, const std::string& scheme, Dictionary options, bool displayOptions);
		bool canOpenURL(const std::string&);
		
		std::string hardwareIdentifier();
		std::string systemVersion();
		
		bool runningOnIPad();
		
		bool musicIsPlaying();
	}
}
