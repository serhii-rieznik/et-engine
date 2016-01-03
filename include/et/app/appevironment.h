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
	struct ApplicationIdentifier;
		
	class Environment
	{
	public: 
		Environment();
				
		const std::string& applicationDocumentsFolder() const
			{ ET_ASSERT(!_documentsFolder.empty()); return _documentsFolder; }
		
		const std::string& applicationPath() const
			{ ET_ASSERT(!_appPath.empty()); return _appPath; }
		
		const std::string& applicationInputDataFolder() const
			{ ET_ASSERT(!_dataFolder.empty()); return _dataFolder; }
		
		const std::string& applicationPackagePath() const
			{ ET_ASSERT(!_appPackagePath.empty()); return _appPackagePath; }
		
		void updateDocumentsFolder(const ApplicationIdentifier& i);
		
	private:
		std::string _appPath;
		std::string _appPackagePath;
		std::string _dataFolder;
		std::string _documentsFolder;
	};
}
