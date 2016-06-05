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
	struct ApplicationIdentifier
	{
		std::string identifier;
		std::string companyName;
		std::string applicationName;

		ApplicationIdentifier() = default;

		ApplicationIdentifier(const std::string& aIdentifier, const std::string& aCompanyName,
			const std::string& aApplicationName)
			: identifier(aIdentifier)
			, companyName(aCompanyName)
			, applicationName(aApplicationName)
		{
		}
	};

	class Environment
	{
	public:
		Environment()
			: _appPath(et::applicationPath())
			, _appPackagePath(et::applicationPackagePath())
			, _dataFolder(et::applicationDataFolder())
		{
		}

		const std::string& applicationDocumentsFolder() const
			{ ET_ASSERT(!_documentsFolder.empty()); return _documentsFolder; }
		
		const std::string& applicationPath() const
			{ ET_ASSERT(!_appPath.empty()); return _appPath; }
		
		const std::string& applicationInputDataFolder() const
			{ ET_ASSERT(!_dataFolder.empty()); return _dataFolder; }
		
		const std::string& applicationPackagePath() const
			{ ET_ASSERT(!_appPackagePath.empty()); return _appPackagePath; }
		
		void updateDocumentsFolder(const ApplicationIdentifier& i)
		{
			_documentsFolder = addTrailingSlash(libraryBaseFolder() + i.companyName);
			if (!folderExists(_documentsFolder))
				createDirectory(_documentsFolder, true);

			ET_ASSERT(folderExists(_documentsFolder));

			_documentsFolder += addTrailingSlash(i.applicationName);
			if (!folderExists(_documentsFolder))
				createDirectory(_documentsFolder, true);

			ET_ASSERT(folderExists(_documentsFolder));
		}
		
	private:
		std::string _appPath;
		std::string _appPackagePath;
		std::string _dataFolder;
		std::string _documentsFolder;
	};
}
