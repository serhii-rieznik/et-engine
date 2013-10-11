/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{
	struct ApplicationIdentifier;
	
	class AppEnvironment
	{
	public: 
		AppEnvironment();

		const std::string& applicationPath() const
			{ return _appPath; }

		const std::string& applicationInputDataFolder() const
			{ return _dataFolder; }

		const std::string& applicationPackagePath() const
			{ return _appPackagePath; }

		void addSearchPath(const std::string& path);
		void addRelativeSearchPath(const std::string& path);
		
		std::string findFile(const std::string& name) const;
		std::string findFolder(const std::string& name) const;
		std::string resolveScalableFileName(const std::string& name, size_t scale) const;

		bool expandFileName(std::string& name) const;
		
		const std::string& applicationDocumentsFolder() const;
		void updateDocumentsFolder(const ApplicationIdentifier& i);

	private:
		std::string _appPath;
		std::string _appPackagePath;
		std::string _dataFolder;
		std::string _documentsFolder;
		StringList _searchPath;
	};
}