/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et/app/appevironment.h>

using namespace et;

AppEnvironment::AppEnvironment() :
	_appPath(et::applicationPath()),
	_appPackagePath(et::applicationPackagePath()),
	_dataFolder(et::applicationDataFolder())
{
	addSearchPath(_appPath);

	addSearchPath(_dataFolder);
    addSearchPath(_dataFolder + "../");
	addSearchPath(_dataFolder + "../../");

	addSearchPath(_dataFolder + "data/");
	addSearchPath(_dataFolder + "../data/");
	addSearchPath(_dataFolder + "../../data/");

	addSearchPath(_dataFolder + "textures/" );
	addSearchPath(_dataFolder + "data/textures/" );
	addSearchPath(_dataFolder + "../data/textures/" );
	addSearchPath(_dataFolder + "../../data/textures/" );

	addSearchPath(_dataFolder + "shaders/" );
	addSearchPath(_dataFolder + "data/shaders/" );
	addSearchPath(_dataFolder + "../data/shaders/" );
	addSearchPath(_dataFolder + "../../data/shaders/" );
}

const std::string& AppEnvironment::applicationDocumentsFolder() const
{
	ET_ASSERT(!_documentsFolder.empty());
	return _documentsFolder;
}

void AppEnvironment::updateDocumentsFolder(const ApplicationIdentifier& i)
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

void AppEnvironment::addSearchPath(const std::string& path)
{
	_searchPath.push_back(addTrailingSlash(normalizeFilePath(path)));
}

void AppEnvironment::addRelativeSearchPath(const std::string& path)
{
	_searchPath.push_back(addTrailingSlash(normalizeFilePath(_appPath + path)));
}

std::string AppEnvironment::findFile(const std::string& name) const
{
	if (fileExists(name)) return name;

	for (auto& i : _searchPath)
	{
		std::string currentName = i + name;
		if (fileExists(currentName))
			return currentName;
	}

	return name;
}

std::string AppEnvironment::findFolder(const std::string& name) const
{
	if (folderExists(name)) return name;
	
	for (auto& i : _searchPath)
	{
		std::string currentName = i + name;
		if (folderExists(currentName))
			return currentName;
	}
	
	return name;	
}

bool AppEnvironment::expandFileName(std::string& name) const
{
	for (auto& i : _searchPath)
	{
		const std::string currentName = i + name;
		if (fileExists(currentName))
		{
			name = currentName;
			return true;
		}
	}

	return false;
}

std::string AppEnvironment::resolveScalableFileName(const std::string& name, size_t scale) const
{
	ET_ASSERT(scale > 0);
	std::string foundFile = findFile(name);

	if ((foundFile.find_last_of("@") != std::string::npos) && fileExists(foundFile))
		return foundFile;

	std::string baseName = removeFileExt(foundFile);
	std::string ext = getFileExt(foundFile);

	while (scale >= 1)
	{
		std::string newFile = findFile(baseName + "@" + intToStr(scale) + "x." + ext);
		if (fileExists(newFile))
			return newFile;

		if (scale == 1)
		{
			newFile = findFile(baseName + "." + ext);
			if (fileExists(newFile))
				return newFile;
		}

		scale -= 1;
	}

	return foundFile;
}
